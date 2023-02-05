/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#include "land-view.hpp"

// Rds
#include "land-view-impl.rds.hpp"

// Revolution Now
#include "anim-builders.hpp"
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "co-time.hpp"
#include "co-wait.hpp"
#include "colony-id.hpp"
#include "compositor.hpp"
#include "imap-updater.hpp"
#include "land-view-anim.hpp"
#include "land-view-render.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "orders.hpp"
#include "physics.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "time.hpp"
#include "ts.hpp"
#include "unit-id.hpp"
#include "unit-mgr.hpp"
#include "viewport.hpp"
#include "visibility.hpp"
#include "window.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.hpp"
#include "ss/ref.hpp"
#include "ss/turn.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/state.hpp"

// C++ standard library
#include <chrono>

using namespace std;

namespace rn {

namespace {

struct RawInput {
  RawInput( LandViewRawInput_t input_ )
    : input( std::move( input_ ) ), when( Clock_t::now() ) {}
  LandViewRawInput_t input;
  Time_t             when;
};

struct PlayerInput {
  PlayerInput( LandViewPlayerInput_t input_, Time_t when_ )
    : input( std::move( input_ ) ), when( when_ ) {}
  LandViewPlayerInput_t input;
  Time_t                when;
};

} // namespace

struct LandViewPlane::Impl : public Plane {
  Planes&          planes_;
  SS&              ss_;
  TS&              ts_;
  Visibility       viz_;
  LandViewAnimator lv_animator_;

  vector<MenuPlane::Deregistrar> dereg;

  co::stream<RawInput>    raw_input_stream_;
  co::stream<PlayerInput> translated_input_stream_;
  LandViewMode_t          landview_mode_ = LandViewMode::none{};

  // Holds info about the previous unit that was asking for or-
  // ders, since it can affect the UI behavior when asking for
  // the current unit's orders (just some niceties that make it
  // easier for the player to accurately control multiple units
  // that are alternately asking for orders).
  struct LastUnitInput {
    UnitId unit_id                  = {};
    bool   need_input_buffer_shield = false;
  };
  maybe<LastUnitInput> last_unit_input_;

  maybe<InputOverrunIndicator> input_overrun_indicator_;

  bool g_needs_scroll_to_unit_on_input = true;

  SmoothViewport const& viewport() const {
    return ss_.land_view.viewport;
  }

  SmoothViewport& viewport() { return ss_.land_view.viewport; }

  void register_menu_items( MenuPlane& menu_plane ) {
    // Register menu handlers.
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::cheat_reveal_map, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::zoom_in, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::zoom_out, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::restore_zoom, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::find_blinking_unit, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::sentry, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::fortify, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::dump, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::plow, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::road, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::hidden_terrain, *this ) );
  }

  Impl( Planes& planes, SS& ss, TS& ts, maybe<e_nation> nation )
    : planes_( planes ),
      ss_( ss ),
      ts_( ts ),
      viz_( Visibility::create( ss, nation ) ),
      lv_animator_( ss, ss.land_view.viewport ) {
    register_menu_items( planes.menu() );
    // Initialize general global data.
    landview_mode_   = LandViewMode::none{};
    last_unit_input_ = nothing;
    raw_input_stream_.reset();
    translated_input_stream_.reset();
    g_needs_scroll_to_unit_on_input = true;
    // This is done to initialize the viewport with info about
    // the viewport size that cannot be known while it is being
    // constructed.
    advance_viewport_state();
  }

  /****************************************************************
  ** Tile Clicking
  *****************************************************************/
  // If there is a single unit on the square with orders and
  // allow_activate is false then the unit's orders will be
  // cleared.
  //
  // If there is a single unit on the square with orders and
  // allow_activate is true then the unit's orders will be
  // cleared and the unit will be placed at the back of the queue
  // to poten- tially move this turn.
  //
  // If there is a single unit on the square with no orders and
  // allow_activate is false then nothing is done.
  //
  // If there is a single unit on the square with no orders and
  // allow_activate is true then the unit will be prioritized
  // (moved to the front of the queue).
  //
  // If there are multiple units on the square then it will pop
  // open a window to allow the user to select and/or activate
  // them, with the results for each unit behaving in a similar
  // way to the single-unit case described above with respect to
  // orders and the allow_activate flag.
  wait<vector<LandViewPlayerInput_t>> click_on_world_tile(
      Coord coord ) {
    vector<LandViewPlayerInput_t> res;
    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    bool allow_activate =
        landview_mode_.holds<LandViewMode::unit_input>();

    // First check for colonies.
    if( auto maybe_id = ss_.colonies.maybe_from_coord( coord );
        maybe_id ) {
      auto& colony = add( LandViewPlayerInput::colony{} );
      colony.id    = *maybe_id;
      co_return res;
    }

    // Now check for units.
    auto const& units =
        euro_units_from_coord_recursive( ss_.units, coord );
    if( units.size() != 0 ) {
      // Decide which units are selected and for what actions.
      vector<UnitSelection> selections;
      if( units.size() == 1 ) {
        auto          id = *units.begin();
        UnitSelection selection{
            id, e_unit_selection::clear_orders };
        if( !ss_.units.unit_for( id ).has_orders() &&
            allow_activate )
          selection.what = e_unit_selection::activate;
        selections = vector{ selection };
      } else {
        selections = co_await unit_selection_box(
            ss_, planes_.window(), units, allow_activate );
      }

      vector<UnitId> prioritize;
      for( auto const& selection : selections ) {
        switch( selection.what ) {
          case e_unit_selection::clear_orders:
            ss_.units.unit_for( selection.id ).clear_orders();
            break;
          case e_unit_selection::activate:
            CHECK( allow_activate );
            // Activation implies also to clear orders if they're
            // not already cleared. We do this here because, even
            // if the prioritization is later denied (because the
            // unit has already moved this turn) the clearing of
            // the orders should still be upheld, because that
            // can always be done, hence they are done
            // separately.
            ss_.units.unit_for( selection.id ).clear_orders();
            prioritize.push_back( selection.id );
            break;
        }
      }
      // These need to all be grouped into a vector so that the
      // first prioritized unit doesn't start asking for orders
      // before the rest are prioritized.
      if( !prioritize.empty() )
        add( LandViewPlayerInput::prioritize{} ).units =
            std::move( prioritize );

      co_return res;
    }

    // Nothing to click on, so just scroll the map to center on
    // the clicked tile.
    viewport().set_point_seek(
        viewport().world_tile_to_world_pixel_center( coord ) );

    co_return res;
  }

  /****************************************************************
  ** Input Processor
  *****************************************************************/
  // Fetches one raw input and translates it, adding a new ele-
  // ment into the "translated" stream. For each translated event
  // created, preserve the time that the corresponding raw input
  // event was received.
  wait<> single_raw_input_translator() {
    RawInput raw_input = co_await raw_input_stream_.next();

    switch( raw_input.input.to_enum() ) {
      using namespace LandViewRawInput;
      case e::reveal_map: {
        co_await cheat_reveal_map( planes_, ss_, ts_ );
        break;
      }
      case e::toggle_map_reveal: {
        cheat_toggle_reveal_full_map( planes_, ss_, ts_ );
        break;
      }
      case e::escape: {
        translated_input_stream_.send( PlayerInput(
            LandViewPlayerInput::exit{}, raw_input.when ) );
        break;
      }
      case e::next_turn: {
        translated_input_stream_.send( PlayerInput(
            LandViewPlayerInput::next_turn{}, raw_input.when ) );
        break;
      }
      case e::orders: {
        translated_input_stream_.send( PlayerInput(
            LandViewPlayerInput::give_orders{
                .orders = raw_input.input
                              .get<LandViewRawInput::orders>()
                              .orders },
            raw_input.when ) );
        break;
      }
      case e::leave_hidden_terrain: {
        SHOULD_NOT_BE_HERE;
      }
      case e::european_status: {
        translated_input_stream_.send(
            PlayerInput( LandViewPlayerInput::european_status{},
                         raw_input.when ) );
        break;
      }
      case e::hidden_terrain: {
        auto new_state = LandViewMode::hidden_terrain{};
        SCOPED_SET_AND_RESTORE( landview_mode_, new_state );
        auto popper = ts_.map_updater.push_options_and_redraw(
            []( MapUpdaterOptions& options ) {
              options.render_forests = false;
              // The original game does not render LCRs or re-
              // sources in the hidden terrain mode, likely be-
              // cause this would allow the player to cheat and
              // see if there is a resource under the forest or
              // LCR tile instead of taking them time and risk
              // to explore those tiles. What is rendered here
              // (the underlying ground terrain) is already
              // given to the player on the side panel (and we
              // don't want to give away anything else).
              options.render_resources = false;
              options.render_lcrs      = false;
            } );
        // Consume further inputs but eat all of them except
        // for the ones we want.
        while( true ) {
          RawInput raw_input = co_await raw_input_stream_.next();
          if( raw_input.input.holds<
                  LandViewRawInput::leave_hidden_terrain>() )
            break;
          if( auto tile_click =
                  raw_input.input
                      .get_if<LandViewRawInput::tile_click>();
              tile_click.has_value() )
            viewport().set_point_seek(
                viewport().world_tile_to_world_pixel_center(
                    tile_click->coord ) );
        }
        break;
      }
      case e::tile_click: {
        auto& o = raw_input.input.get<tile_click>();
        if( o.mods.shf_down ) {
          // cheat mode.
          maybe<e_nation> nation = active_player( ss_.turn );
          if( !nation.has_value() ) break;
          co_await cheat_create_unit_on_map( ss_, ts_, *nation,
                                             o.coord );
          break;
        }
        vector<LandViewPlayerInput_t> inputs =
            co_await click_on_world_tile( o.coord );
        // Since we may have just popped open a box to ask the
        // user to select units, just use the "now" time so
        // that these events don't get disgarded. Also, mouse
        // clicks are not likely to get buffered for too long
        // anyway.
        for( auto const& input : inputs )
          translated_input_stream_.send(
              PlayerInput( input, Clock_t::now() ) );
        break;
      }
      case e::center: {
        // For this one, we just perform the action right here.
        using u_i = LandViewMode::unit_input;
        auto blinking_unit =
            landview_mode_.get_if<u_i>().member( &u_i::unit_id );
        if( !blinking_unit ) {
          lg.warn(
              "There are no units currently asking for "
              "orders." );
          break;
        }
        Coord const tile = coord_for_unit_indirect_or_die(
            ss_.units, *blinking_unit );
        co_await center_on_tile( tile );
        break;
      }
    }
  }

  wait<LandViewPlayerInput_t> next_player_input_object() {
    while( true ) {
      while( !translated_input_stream_.ready() )
        co_await single_raw_input_translator();
      PlayerInput res = co_await translated_input_stream_.next();
      // Ignore any input events that are too old.
      if( Clock_t::now() - res.when < chrono::seconds{ 2 } )
        co_return std::move( res.input );
    }
  }

  /****************************************************************
  ** Land View Plane
  *****************************************************************/
  void advance_viewport_state() {
    UNWRAP_CHECK(
        viewport_rect_pixels,
        compositor::section( compositor::e_section::viewport ) );

    viewport().advance_state( viewport_rect_pixels );

    // TODO: should only do the following when the viewport has
    // input focus.
    auto const* __state = ::SDL_GetKeyboardState( nullptr );

    // Returns true if key is pressed.
    auto state = [__state]( ::SDL_Scancode code ) {
      return __state[code] != 0;
    };

    if( state( ::SDL_SCANCODE_LSHIFT ) ) {
      viewport().set_x_push( state( ::SDL_SCANCODE_A )
                                 ? e_push_direction::negative
                             : state( ::SDL_SCANCODE_D )
                                 ? e_push_direction::positive
                                 : e_push_direction::none );
      // y motion
      viewport().set_y_push( state( ::SDL_SCANCODE_W )
                                 ? e_push_direction::negative
                             : state( ::SDL_SCANCODE_S )
                                 ? e_push_direction::positive
                                 : e_push_direction::none );

      if( state( ::SDL_SCANCODE_A ) ||
          state( ::SDL_SCANCODE_D ) ||
          state( ::SDL_SCANCODE_W ) ||
          state( ::SDL_SCANCODE_S ) )
        viewport().stop_auto_panning();
    }
  }

  maybe<orders_t> try_orders_from_lua( int  keycode,
                                       bool ctrl_down,
                                       bool shf_down ) {
    lua::state& st = ts_.lua;

    lua::any lua_orders = st["land_view"]["key_to_orders"](
        keycode, ctrl_down, shf_down );
    if( !lua_orders ) return nothing;

    // FIXME: the conversion from Lua to C++ below needs to be
    // au- tomated once we have sumtype and struct reflection.
    // When this is done then ideally we will just be able to do
    // this:
    //
    //   orders_t orders = lua_orders.as<orders_t>();
    //
    // And it should do the correct conversion and error
    // checking.
    lua::table tbl = lua_orders.as<lua::table>();
    orders_t   orders;
    if( false )
      ;
    else if( tbl["wait"] )
      orders = orders::wait{};
    else if( tbl["forfeight"] )
      orders = orders::forfeight{};
    else if( tbl["build"] )
      orders = orders::build{};
    else if( tbl["fortify"] )
      orders = orders::fortify{};
    else if( tbl["sentry"] )
      orders = orders::sentry{};
    else if( tbl["disband"] )
      orders = orders::disband{};
    else if( tbl["road"] )
      orders = orders::road{};
    else if( tbl["plow"] )
      orders = orders::plow{};
    else if( tbl["move"] ) {
      e_direction d = tbl["move"]["d"].as<e_direction>();
      orders        = orders::move{ .d = d };
    } else {
      FATAL(
          "invalid orders::move object received from "
          "lua." );
    }
    return orders;
  }

  bool covers_screen() const override { return true; }

  void advance_state() override { advance_viewport_state(); }

  void draw( rr::Renderer& renderer ) const override {
    UNWRAP_CHECK(
        viewport_rect_pixels,
        compositor::section( compositor::e_section::viewport ) );
    LandViewRenderer const lv_renderer(
        ss_, renderer, lv_animator_, viz_,
        last_unit_input_.member( &LastUnitInput::unit_id ),
        viewport_rect_pixels, input_overrun_indicator_,
        viewport() );

    lv_renderer.render_non_entities();
    if( landview_mode_.holds<LandViewMode::hidden_terrain>() )
      return;
    lv_renderer.render_entities();
  }

  maybe<function<void()>> menu_click_handler(
      e_menu_item item ) {
    // These are factors by which the zoom will be scaled when
    // zooming in/out with the menus.
    double constexpr zoom_in_factor  = 2.0;
    double constexpr zoom_out_factor = 1.0 / zoom_in_factor;
    // This is so that a zoom-in followed by a zoom-out will re-
    // store to previous state.
    static_assert( zoom_in_factor * zoom_out_factor == 1.0 );
    switch( item ) {
      case e_menu_item::cheat_reveal_map: {
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::reveal_map{} ) );
        };
        return handler;
      }
      case e_menu_item::zoom_in: {
        auto handler = [this] {
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          viewport().smooth_zoom_target( viewport().get_zoom() *
                                         zoom_in_factor );
        };
        return handler;
      }
      case e_menu_item::zoom_out: {
        auto handler = [this] {
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          viewport().smooth_zoom_target( viewport().get_zoom() *
                                         zoom_out_factor );
        };
        return handler;
      }
      case e_menu_item::restore_zoom: {
        if( viewport().get_zoom() == 1.0 ) break;
        auto handler = [this] {
          viewport().smooth_zoom_target( 1.0 );
        };
        return handler;
      }
      case e_menu_item::find_blinking_unit: {
        if( !landview_mode_.holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::center{} ) );
        };
        return handler;
      }
      case e_menu_item::sentry: {
        if( !landview_mode_.holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::orders{
                  .orders = orders::sentry{} } ) );
        };
        return handler;
      }
      case e_menu_item::fortify: {
        if( !landview_mode_.holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::orders{
                  .orders = orders::fortify{} } ) );
        };
        return handler;
      }
      case e_menu_item::dump: {
        if( !landview_mode_.holds<LandViewMode::unit_input>() )
          break;
        // Only for things that can carry cargo (ships and wagon
        // trains).
        if( ss_.units
                .unit_for( landview_mode_
                               .get<LandViewMode::unit_input>()
                               .unit_id )
                .desc()
                .cargo_slots == 0 )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::orders{
                  .orders = orders::dump{} } ) );
        };
        return handler;
      }
      case e_menu_item::plow: {
        if( !landview_mode_.holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::orders{
                  .orders = orders::plow{} } ) );
        };
        return handler;
      }
      case e_menu_item::road: {
        if( !landview_mode_.holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::orders{
                  .orders = orders::road{} } ) );
        };
        return handler;
      }
      case e_menu_item::hidden_terrain: {
        if( landview_mode_
                .holds<LandViewMode::hidden_terrain>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::hidden_terrain{} ) );
        };
        return handler;
      }
      default:
        break;
    }
    return nothing;
  }

  bool will_handle_menu_click( e_menu_item item ) override {
    return menu_click_handler( item ).has_value();
  }

  void handle_menu_click( e_menu_item item ) override {
    DCHECK( menu_click_handler( item ).has_value() );
    ( *menu_click_handler( item ) )();
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::unknown_event: //
        break;
      case input::e_input_event::quit_event:    //
        break;
      case input::e_input_event::win_event:     //
        break;
      case input::e_input_event::key_event: {
        auto& key_event = event.get<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          break;
        handled = e_input_handled::yes;
        if( !input::is_mod_key( key_event ) &&
            landview_mode_
                .holds<LandViewMode::hidden_terrain>() ) {
          raw_input_stream_.send( RawInput(
              LandViewRawInput::leave_hidden_terrain{} ) );
          break;
        }
        // First allow the Lua hook to handle the key press if it
        // wants.
        maybe<orders_t> lua_orders = try_orders_from_lua(
            key_event.keycode, key_event.mod.ctrl_down,
            key_event.mod.shf_down );
        if( lua_orders ) {
          // lg.debug( "received key from lua: {}", lua_orders );
          raw_input_stream_.send(
              RawInput( LandViewRawInput::orders{
                  .orders = *lua_orders } ) );
          break;
        }
        switch( key_event.keycode ) {
          case ::SDLK_z: {
            if( key_event.mod.shf_down )
              viewport().smooth_zoom_target(
                  viewport().optimal_min_zoom() );
            else {
              // If the map surroundings are visible then that
              // means that we are significantly zoomed out, so
              // it is likely that, when zooming in, the user
              // will want to zoom in on the current blinking
              // unit.
              bool center_on_unit =
                  viewport().are_surroundings_visible();
              viewport().smooth_zoom_target( 1.0 );
              if( center_on_unit ) {
                auto blinking_unit =
                    landview_mode_
                        .get_if<LandViewMode::unit_input>();
                if( blinking_unit.has_value() )
                  viewport().set_point_seek(
                      viewport()
                          .world_tile_to_world_pixel_center(
                              coord_for_unit_indirect_or_die(
                                  ss_.units,
                                  blinking_unit->unit_id ) ) );
              }
            }
            break;
          }
          case ::SDLK_w:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::wait{} } ) );
            break;
          case ::SDLK_s:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::sentry{} } ) );
            break;
          case ::SDLK_f:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::fortify{} } ) );
            break;
          case ::SDLK_o:
            // Capital O.
            if( !key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::dump{} } ) );
            break;
          case ::SDLK_b:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::build{} } ) );
            break;
          case ::SDLK_c:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::center{} ) );
            break;
          case ::SDLK_d:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::disband{} } ) );
            break;
          case ::SDLK_h:
            if( !key_event.mod.shf_down ) break;
            if( landview_mode_
                    .holds<LandViewMode::hidden_terrain>() )
              break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::hidden_terrain{} ) );
            break;
          case ::SDLK_r:
            // Cheat function -- reveal entire map.
            if( !key_event.mod.shf_down ) break;
            raw_input_stream_.send( RawInput(
                LandViewRawInput::toggle_map_reveal{} ) );
            break;
          case ::SDLK_e:
            raw_input_stream_.send( RawInput(
                LandViewRawInput::european_status{} ) );
            break;
          case ::SDLK_ESCAPE:
            raw_input_stream_.send(
                RawInput( LandViewRawInput::escape{} ) );
            break;
          case ::SDLK_SPACE:
          case ::SDLK_KP_5:
            if( landview_mode_
                    .holds<LandViewMode::unit_input>() ) {
              if( key_event.mod.shf_down ) break;
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::orders{
                      .orders = orders::forfeight{} } ) );
            } else if( landview_mode_
                           .holds<LandViewMode::none>() ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::next_turn{} ) );
            }
            break;
          default:
            if( key_event.mod.shf_down ) break;
            handled = e_input_handled::no;
            if( key_event.direction ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::orders{
                      .orders = orders::move{
                          *key_event.direction } } ) );
              handled = e_input_handled::yes;
            }
            break;
        }
        break;
      }
      case input::e_input_event::mouse_wheel_event: {
        auto& val = event.get<input::mouse_wheel_event_t>();
        // If the mouse is in the viewport and its a wheel event
        // then we are in business.
        UNWRAP_CHECK( viewport_rect_pixels,
                      compositor::section(
                          compositor::e_section::viewport ) );
        if( val.pos.is_inside( viewport_rect_pixels ) ) {
          if( val.wheel_delta < 0 )
            viewport().set_zoom_push( e_push_direction::negative,
                                      nothing );
          if( val.wheel_delta > 0 )
            viewport().set_zoom_push( e_push_direction::positive,
                                      val.pos );
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          handled = e_input_handled::yes;
        }
        break;
      }
      case input::e_input_event::mouse_button_event: {
        auto& val = event.get<input::mouse_button_event_t>();
        if( val.buttons != input::e_mouse_button_event::left_up )
          break;
        UNWRAP_BREAK(
            world_tile,
            viewport().screen_pixel_to_world_tile( val.pos ) );
        handled = e_input_handled::yes;
        lg.debug( "clicked on tile: {}.", world_tile );
        raw_input_stream_.send(
            RawInput( LandViewRawInput::tile_click{
                .coord = world_tile, .mods = val.mod } ) );
        break;
      }
      default: //
        break;
    }
    return handled;
  }

  /**************************************************************
  ** Dragging
  ***************************************************************/
  struct DragUpdate {
    Coord prev;
    Coord current;
  };
  // Here, `nothing` is used to indicate that it has ended. NOTE:
  // this needs to have update() called on it in the plane's
  // advance_state method.
  co::finite_stream<DragUpdate> drag_stream;
  // The waitable will be waiting on the drag_stream, so it must
  // come after so that it gets destroyed first.
  maybe<wait<>> drag_thread;
  bool          drag_finished = true;

  wait<> dragging( input::e_mouse_button /*button*/,
                   Coord /*origin*/ ) {
    SCOPE_EXIT( drag_finished = true );
    while( maybe<DragUpdate> d = co_await drag_stream.next() )
      viewport().pan_by_screen_coords( d->prev - d->current );
  }

  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( !drag_finished ) return Plane::e_accept_drag::swallow;
    if( button == input::e_mouse_button::r &&
        viewport().screen_coord_in_viewport( origin ) ) {
      viewport().stop_auto_panning();
      drag_stream.reset();
      drag_finished = false;
      drag_thread   = dragging( button, origin );
      return Plane::e_accept_drag::yes;
    }
    return Plane::e_accept_drag::no;
  }

  void on_drag( input::mod_keys const& /*unused*/,
                input::e_mouse_button /*unused*/,
                Coord /*unused*/, Coord prev,
                Coord current ) override {
    drag_stream.send(
        DragUpdate{ .prev = prev, .current = current } );
  }

  void on_drag_finished( input::mod_keys const& /*mod*/,
                         input::e_mouse_button /*button*/,
                         Coord /*origin*/,
                         Coord /*end*/ ) override {
    drag_stream.finish();
    // At this point we assume that the callback will finish on
    // its own after doing any post-drag stuff it needs to do.
  }

  void reset_input_buffers() {
    raw_input_stream_.reset();
    translated_input_stream_.reset();
  }

  void start_new_turn() {
    // An example of why this is needed is because when a unit is
    // moving (say, it is the only active unit) and the screen
    // scrolls away from it to show a colony update, then when
    // that update message closes and the next turn starts and
    // focuses again on the unit, it would not scroll back to
    // that unit.
    g_needs_scroll_to_unit_on_input = true;
  }

  wait<> center_on_tile( Coord coord ) {
    co_await viewport().center_on_tile_smooth( coord );
  }

  void set_visibility( maybe<e_nation> nation ) {
    viz_ = Visibility::create( ss_, nation );
  }

  void zoom_out_full() {
    viewport().set_zoom( viewport().optimal_min_zoom() );
  }

  maybe<UnitId> unit_blinking() {
    return landview_mode_.get_if<LandViewMode::unit_input>()
        .member( &LandViewMode::unit_input::unit_id );
  }

  wait<> eat_cross_unit_buffered_input_events( UnitId id ) {
    if( !config_land_view.input_overrun_detection.enabled )
      co_return;
    reset_input_buffers();
    auto const kWait =
        config_land_view.input_overrun_detection.wait_time;
    SCOPE_EXIT( input_overrun_indicator_ = nothing );
    int const kMaxInputsToWithold =
        config_land_view.input_overrun_detection
            .max_inputs_to_withold;
    for( int i = 0; i < kMaxInputsToWithold; ++i ) {
      auto const res = co_await co::first(
          raw_input_stream_.next(), wait_for_duration( kWait ) );
      if( res.index() == 1 ) // timeout
        break;
      UNWRAP_CHECK( raw_input, res.get_if<RawInput>() );
      auto orders =
          raw_input.input.get_if<LandViewRawInput::orders>();
      if( !orders.has_value() ) {
        raw_input_stream_.reset();
        raw_input_stream_.send( raw_input );
        co_return;
      }
      if( Clock_t::now() - raw_input.when >= kWait ) break;
      // The player may still be trying to move the old unit, so
      // eat this command and display an indicator to signal that
      // inputs are getting eaten.
      input_overrun_indicator_ = InputOverrunIndicator{
          .unit_id = id, .start_time = raw_input.when };
    }
    reset_input_buffers();
  }

  bool is_unit_visible_on_map( GenericUnitId id ) const {
    if( lv_animator_.unit_animations().contains( id ) )
      return true;
    maybe<Coord> const tile =
        coord_for_unit_multi_ownership( ss_, id );
    if( !tile.has_value() ) return false;
    if( ss_.colonies.maybe_from_coord( *tile ).has_value() )
      // Note that if the unit is in a colony square and is de-
      // fending an attack (and hence visible) then the anima-
      // tions check above should have caught it.
      return false;
    if( ss_.units.unit_kind( id ) == e_unit_kind::euro ) {
      if( is_unit_onboard( ss_.units,
                           ss_.units.check_euro_unit( id ) ) )
        // Note that if the unit is onboard but is asking for or-
        // ders then the animations check above should have
        // caught it.
        return false;
    }
    vector<GenericUnitId> const sorted = land_view_unit_stack(
        ss_, *tile,
        last_unit_input_.member( &LastUnitInput::unit_id ) );
    CHECK( !sorted.empty() );
    return ( sorted[0] == id );
  }

  wait<LandViewPlayerInput_t> get_next_input( UnitId id ) {
    // There are some things that use last_unit_input_ below that
    // will crash if the last unit that asked for orders no
    // longer exists. So we'll just do this up here to be safe.
    if( last_unit_input_.has_value() &&
        !ss_.units.exists( last_unit_input_->unit_id ) )
      last_unit_input_ = nothing;

    // We only pan to the unit here because if we did that out-
    // side of this if statement then the viewport would pan to
    // the blinking unit after the player e.g. clicks on another
    // unit to activate it.
    if( !last_unit_input_.has_value() ||
        last_unit_input_->unit_id != id )
      g_needs_scroll_to_unit_on_input = true;

    // This might be true either because we started a new turn,
    // or because of the above assignment.
    if( g_needs_scroll_to_unit_on_input )
      co_await lv_animator_.ensure_visible_unit( id );
    g_needs_scroll_to_unit_on_input = false;

    // Need to set this before taking any user input, otherwise
    // we will be in the "none" state, which represents the
    // end-of-turn, in which case e.g. a space bar gets sent as a
    // "next turn" event as opposed to a "forfeight" event (which
    // can happen because sometimes we take input while eating
    // buffered input events below).
    SCOPED_SET_AND_RESTORE(
        landview_mode_,
        LandViewMode::unit_input{ .unit_id = id } );

    // When we start on a new unit clear the input queue so that
    // commands that were accidentally buffered while controlling
    // the previous unit don't affect this new one, which would
    // almost certainly not be desirable. This function does that
    // clearing in an interactive way in order to signal to the
    // user what is happening.
    bool const eat_buffered =
        last_unit_input_.has_value() &&
        // If still moving same unit then no need to shield.
        last_unit_input_->unit_id != id &&
        last_unit_input_->need_input_buffer_shield &&
        // E.g. if the previous unit got on a ship then the
        // player doesn't expect them to continue moving, so no
        // need to shield this unit.
        is_unit_on_map( ss_.units, last_unit_input_->unit_id );
    if( eat_buffered ) {
      // We typically wait for a bit while eating input events;
      // during that time, make sure that the unit who is about
      // to ask for orders is rendered on the front.
      AnimationSequence const seq = anim_seq_unit_to_front( id );
      wait<> anim = lv_animator_.animate_sequence( seq );
      co_await eat_cross_unit_buffered_input_events( id );
    }

    // Most of the time we start the blinking with the unit in-
    // visible, that way the start of the animation creates an
    // immediate visual change that draws the player's eye to the
    // unit. However, in some cases, the unit is not initially
    // visible even before the animation starts; this could be
    // e.g. because it is on a colony square, it is the cargo of
    // a ship, or it is in the middle of a stack of units. In
    // those cases, we want the blinking animation to be ini-
    // tially visible so that it creates a similar visual change
    // to draw the player's eye. Note that if we did eat buffered
    // input events above then the unit will have been made vis-
    // ible for a brief time regardless, so in that case we know
    // we that we should start with the unit invisible. Note that
    // this needs to be computed before we change the last unit
    // input, because the last unit input is involved in deter-
    // mining whether or not the unit is currently visible (that
    // is because the algorithm that orders unit stacks on a tile
    // will treat the last unit input specially, putting it on
    // top for various UX reasons).
    bool const visible_initially =
        !is_unit_visible_on_map( id ) && !eat_buffered;
    lg.trace( "visible={}, eat={}, init={}",
              is_unit_visible_on_map( id ), eat_buffered,
              visible_initially );

    if( !last_unit_input_.has_value() ||
        last_unit_input_->unit_id != id )
      last_unit_input_ = LastUnitInput{
          .unit_id = id, .need_input_buffer_shield = true };

    // Run the blinker while waiting for user input. The question
    // is, do we want the blinking to start "on" or "off"? The
    // idea is that we want to start it off in the opposite vi-
    // sual state that the unit currently is in.
    LandViewPlayerInput_t input = co_await co::background(
        next_player_input_object(),
        lv_animator_.animate_blink( id, visible_initially ) );

    // Disable input buffer shielding when the unit issues a
    // non-move command, because in that case it is unlikely that
    // the player will assume that the subsequent command they
    // issue would go to the same unit. E.g., if the unit issues
    // a command to fortify this unit, then they know that the
    // next command they issue would go to the new unit, so we
    // don't need to shield it.
    if( auto give_orders =
            input.get_if<LandViewPlayerInput::give_orders>();
        give_orders.has_value() &&
        !give_orders->orders.holds<orders::move>() ) {
      if( last_unit_input_.has_value() )
        last_unit_input_->need_input_buffer_shield = false;
    }

    co_return input;
  }

  wait<LandViewPlayerInput_t> eot_get_next_input() {
    last_unit_input_ = nothing;
    landview_mode_   = LandViewMode::none{};
    return next_player_input_object();
  }
};

/****************************************************************
** LandViewPlane
*****************************************************************/
Plane& LandViewPlane::impl() { return *impl_; }

LandViewPlane::~LandViewPlane() = default;

LandViewPlane::LandViewPlane( Planes& planes, SS& ss, TS& ts,
                              maybe<e_nation> visibility )
  : impl_( new Impl( planes, ss, ts, visibility ) ) {}

wait<> LandViewPlane::ensure_visible( Coord const& coord ) {
  return impl_->lv_animator_.ensure_visible( coord );
}

wait<> LandViewPlane::center_on_tile( Coord coord ) {
  return impl_->center_on_tile( coord );
}

void LandViewPlane::set_visibility( maybe<e_nation> nation ) {
  return impl_->set_visibility( nation );
}

wait<> LandViewPlane::ensure_visible_unit( GenericUnitId id ) {
  return impl_->lv_animator_.ensure_visible_unit( id );
}

wait<LandViewPlayerInput_t> LandViewPlane::get_next_input(
    UnitId id ) {
  return impl_->get_next_input( id );
}

wait<LandViewPlayerInput_t> LandViewPlane::eot_get_next_input() {
  return impl_->eot_get_next_input();
}

void LandViewPlane::reset_input_buffers() {
  return impl_->reset_input_buffers();
}

void LandViewPlane::start_new_turn() {
  return impl_->start_new_turn();
}

void LandViewPlane::zoom_out_full() {
  return impl_->zoom_out_full();
}

maybe<UnitId> LandViewPlane::unit_blinking() {
  return impl_->unit_blinking();
}

wait<> LandViewPlane::animate( AnimationSequence const& seq ) {
  return impl_->lv_animator_.animate_sequence( seq );
}

} // namespace rn
