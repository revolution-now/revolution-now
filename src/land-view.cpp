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
#include "command.hpp"
#include "compositor.hpp"
#include "hidden-terrain.hpp"
#include "imap-updater.hpp"
#include "imenu-server.hpp"
#include "land-view-anim.hpp"
#include "land-view-render.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "physics.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "roles.hpp"
#include "society.hpp"
#include "time.hpp"
#include "ts.hpp"
#include "unit-id.hpp"
#include "unit-mgr.hpp"
#include "viewport.hpp"
#include "visibility.hpp"
#include "white-box.hpp"
#include "window.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/menu-items.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/coord.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp" // IWYU pragma: keep

// base
#include "base/scope-exit.hpp"

// luapp
#include "luapp/enum.hpp" // IWYU pragma: keep
#include "luapp/state.hpp"

// C++ standard library
#include <chrono>
#include <queue>

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::gfx::rect;

struct RawInput {
  RawInput( LandViewRawInput input_ )
    : input( std::move( input_ ) ), when( Clock_t::now() ) {}
  LandViewRawInput input;
  Time_t when;
};

struct PlayerInput {
  PlayerInput( LandViewPlayerInput input_, Time_t when_ )
    : input( std::move( input_ ) ), when( when_ ) {}
  LandViewPlayerInput input;
  Time_t when;
};

// Holds info about the previous unit that was asking for orders,
// since it can affect the UI behavior when asking for the cur-
// rent unit's orders (just some niceties that make it easier for
// the player to accurately control multiple units that are al-
// ternately asking for orders).
//
// This class also functions to guard against usage of unit IDs
// representing units that no longer exist, which is a bit hard
// to track otherwise.
struct LastUnitInput {
  [[maybe_unused]] explicit LastUnitInput( SSConst const& ss,
                                           UnitId const id )
    : ss_( ss ), unit_id_( id ) {}

  maybe<UnitId> unit_id() const {
    maybe<UnitId> res;
    if( ss_.units.exists( unit_id_ ) ) res = unit_id_;
    return res;
  }

 private:
  SSConst const& ss_;
  // Should not allow direct access to this because it could rep-
  // resent a unit that no longer exists.
  UnitId unit_id_ = {};

 public:
  bool need_input_buffer_shield = false;
  // Records the total number of windows that have been opened
  // thus far when the unit last asked for orders. That way, if
  // the same unit asks for orders again, we can check this to
  // see if any windows were opened during the course of the last
  // move and, if so, clear the input buffers.
  int window_count = 0;
};

} // namespace

struct LandViewPlane::Impl : public IPlane {
  SS& ss_;
  TS& ts_;
  unique_ptr<IVisibility const> viz_;
  LandViewAnimator animator_;

  vector<MenuPlane::Deregistrar> dereg;
  vector<IMenuServer::Deregistrar> dereg2;

  co::stream<RawInput> raw_input_stream_;
  queue<PlayerInput> translated_input_stream_;
  LandViewMode mode_ = LandViewMode::none{};

  maybe<co::stream<point>> white_box_stream_;

  maybe<LastUnitInput> last_unit_input_;

  // For convenience.
  maybe<UnitId> last_unit_input_id() const {
    if( !last_unit_input_.has_value() ) return nothing;
    return last_unit_input_->unit_id();
  }

  maybe<InputOverrunIndicator> input_overrun_indicator_;

  bool g_needs_scroll_to_unit_on_input = true;

  SmoothViewport const& viewport() const {
    return ss_.land_view.viewport;
  }

  SmoothViewport& viewport() { return ss_.land_view.viewport; }

  void register_menu_items( MenuPlane& menu_plane,
                            IMenuServer& menu_server ) {
    // Register menu handlers.
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::cheat_create_unit_on_map, *this ) );
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
    dereg2.push_back( menu_server.register_handler(
        e_menu_item::sentry, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::fortify, *this ) );
    dereg2.push_back( menu_server.register_handler(
        e_menu_item::fortify, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::disband, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::wait, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::build_colony, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::return_to_europe, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::dump, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::plow, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::road, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::activate, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::hidden_terrain, *this ) );
    dereg.push_back( menu_plane.register_handler(
        e_menu_item::toggle_view_mode, *this ) );
  }

  Impl( SS& ss, TS& ts, maybe<e_nation> nation )
    : ss_( ss ),
      ts_( ts ),
      viz_( create_visibility_for( ss, nothing ) ),
      animator_( ss, ss.land_view.viewport, viz_ ) {
    set_visibility( nation );
    CHECK( viz_ != nullptr );
    register_menu_items( ts.planes.get().menu,
                         ts.planes.get().menu2 );
    // Initialize general global data.
    mode_            = LandViewMode::none{};
    last_unit_input_ = nothing;
    raw_input_stream_.reset();
    translated_input_stream_        = {};
    g_needs_scroll_to_unit_on_input = true;
    // This is done to initialize the viewport with info about
    // the viewport size that cannot be known while it is being
    // constructed.
    advance_viewport_state();
  }

  maybe<point> white_box_tile_if_visible() const {
    bool visible = false;
    SWITCH( mode_ ) {
      CASE( none ) { break; }
      CASE( end_of_turn ) {
        visible = true;
        break;
      }
      CASE( hidden_terrain ) {
        visible = true;
        break;
      }
      CASE( unit_input ) { break; }
      CASE( view_mode ) {
        visible = true;
        break;
      }
    }
    if( visible ) return white_box_tile( ss_ );
    return nothing;
  }

  maybe<point> find_tile_to_center_on() const {
    // First try white box.
    if( auto const p = white_box_tile_if_visible();
        p.has_value() )
      return *p;

    // Next try blinking units.
    SWITCH( mode_ ) {
      CASE( none ) { break; }
      CASE( end_of_turn ) { break; }
      CASE( hidden_terrain ) { break; }
      CASE( unit_input ) {
        return coord_for_unit_indirect_or_die(
            ss_.units, unit_input.unit_id );
      }
      CASE( view_mode ) { break; }
    }

    return nothing;
  }

  /****************************************************************
  ** Tile Clicking
  *****************************************************************/
  // If there is a single unit on the square with orders then the
  // unit's orders will be cleared and the unit will be placed at
  // the back of the queue to potentially move this turn.
  //
  // If there is a single unit on the square with no orders then
  // the unit will be prioritized (moved to the front of the
  // queue).
  //
  // If there are multiple units on the square then it will pop
  // open a window to allow the user to select and/or activate
  // them, with the results for each unit behaving in a similar
  // way to the single-unit case described above.
  wait<vector<LandViewPlayerInput>> click_on_world_tile(
      Coord coord ) {
    vector<LandViewPlayerInput> res;

    auto scroll_map = [&] {
      // Nothing to click on, so just scroll the map to center on
      // the clicked tile.
      viewport().set_point_seek(
          viewport().world_tile_to_world_pixel_center( coord ) );
    };

    if( white_box_stream_.has_value() )
      white_box_stream_->send( coord );

    if( mode_.holds<LandViewMode::hidden_terrain>() ) {
      scroll_map();
      co_return res;
    }

    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    // First check for colonies.
    // FIXME: limit this using the roles module, and also in the
    // enter_on_world_tile method below.
    if( auto maybe_id = ss_.colonies.maybe_from_coord( coord );
        maybe_id ) {
      auto& colony = add( LandViewPlayerInput::colony{} );
      colony.id    = *maybe_id;
      co_return res;
    }

    // Now check for units.
    bool const allow_unit_click =
        mode_.holds<LandViewMode::unit_input>() ||
        mode_.holds<LandViewMode::view_mode>() ||
        mode_.holds<LandViewMode::end_of_turn>();
    auto const units =
        euro_units_from_coord_recursive( ss_.units, coord );
    if( allow_unit_click && units.size() != 0 ) {
      // Decide which units are selected and for what actions.
      vector<UnitSelection> selections;
      if( units.size() == 1 ) {
        auto id = *units.begin();
        UnitSelection selection{
          id, e_unit_selection::clear_orders };
        if( !ss_.units.unit_for( id ).has_orders() )
          selection.what = e_unit_selection::activate;
        selections = vector{ selection };
      } else {
        selections = co_await unit_selection_box(
            ss_, ts_.planes.get().window, units );
      }

      vector<UnitId> prioritize;
      for( auto const& selection : selections ) {
        switch( selection.what ) {
          case e_unit_selection::clear_orders:
            ss_.units.unit_for( selection.id ).clear_orders();
            break;
          case e_unit_selection::activate:
            // Activation implies also to clear orders if they're
            // not already cleared. We do this here because, even
            // if the prioritization is later denied (because the
            // unit has already moved this turn) the clearing of
            // the orders should still be upheld, because that
            // can always be done, hence they are done sepa-
            // rately.
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

    scroll_map();
    co_return res;
  }

  wait<vector<LandViewPlayerInput>> right_click_on_world_tile(
      gfx::point const tile ) {
    vector<LandViewPlayerInput> res;

    if( white_box_stream_.has_value() )
      white_box_stream_->send( tile );

    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    if( mode_.holds<LandViewMode::unit_input>() ) {
      add( LandViewPlayerInput::toggle_view_mode{
        .options = ViewModeOptions{ .initial_tile = tile } } );
      co_return res;
    }

    co_return res;
  }

  wait<> context_menu( point const where,
                       point const /*tile*/ ) {
    viewport().stop_auto_zoom();
    viewport().stop_auto_panning();
    MenuContents const orders_contents{
      .groups = {
        MenuItemGroup{
          .elems =
              {
                MenuElement::leaf{ .item =
                                       e_menu_item::fortify },
                MenuElement::leaf{ .item = e_menu_item::sentry },
                MenuElement::leaf{ .item = e_menu_item::dump },
              } },
      } };
    MenuContents const zoom_contents{
      .groups = {
        MenuItemGroup{
          .elems =
              {
                MenuElement::node{ .text = "Orders",
                                   .menu = orders_contents },
                MenuElement::leaf{ .item =
                                       e_menu_item::zoom_in },
                MenuElement::leaf{ .item =
                                       e_menu_item::zoom_out },
              } },
      } };
    MenuContents const contents{
      .groups = {
        MenuItemGroup{
          .elems =
              {
                MenuElement::leaf{ .item =
                                       e_menu_item::fortify },
                MenuElement::leaf{ .item = e_menu_item::sentry },
                MenuElement::leaf{ .item =
                                       e_menu_item::disband },
                MenuElement::leaf{ .item = e_menu_item::dump },
              } },
        MenuItemGroup{
          .elems =
              {
                MenuElement::node{ .text = "Zoom",
                                   .menu = zoom_contents },
              } },
        MenuItemGroup{
          .elems =
              {
                MenuElement::leaf{ .item = e_menu_item::plow },
                MenuElement::leaf{ .item = e_menu_item::road },
                MenuElement::leaf{
                  .item = e_menu_item::build_colony },
              } },
        MenuItemGroup{
          .elems =
              {
                MenuElement::node{ .text = "Zoom2",
                                   .menu = zoom_contents },
              } },
      } };
    MenuAllowedPositions const positions{
      .positions_allowed = { { .where = where } } };

    auto& menu_server = ts_.planes.get().menu2.typed();
    auto const selected_item =
        co_await menu_server.open_menu( contents, positions );
    if( !selected_item.has_value() ) co_return;
    menu_server.click_item( *selected_item );
  }

  /****************************************************************
  ** Tile Entering
  *****************************************************************/
  // This happens typically when the white box is visible and the
  // enter or return keys are hit on a square.
  wait<vector<LandViewPlayerInput>> enter_on_world_tile(
      gfx::point const tile ) {
    vector<LandViewPlayerInput> res;

    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    // First check for colonies.
    // FIXME: limit this using the roles module, and also in the
    // click_on_world_tile method above.
    if( auto const colony_id =
            ss_.colonies.maybe_from_coord( tile );
        colony_id.has_value() ) {
      add( LandViewPlayerInput::colony{ .id = *colony_id } );
      co_return res;
    }

    co_return res;
  }

  static vector<LandViewPlayerInput> activate_tile(
      point const tile ) {
    vector<LandViewPlayerInput> res;
    auto& activate =
        res.emplace_back()
            .emplace<LandViewPlayerInput::activate>();
    activate.tile = tile;
    return res;
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
      using e = LandViewRawInput::e;
      case e::reveal_map: {
        if( !cheat_mode_enabled( ss_ ) ) break;
        co_await cheat_reveal_map( ss_, ts_ );
        break;
      }
      case e::toggle_map_reveal: {
        if( !cheat_mode_enabled( ss_ ) ) break;
        cheat_toggle_reveal_full_map( ss_, ts_ );
        break;
      }
      case e::cheat_create_unit: {
        if( !cheat_mode_enabled( ss_ ) ) break;
        maybe<e_nation> const nation =
            player_for_role( ss_, e_player_role::active );
        if( !nation.has_value() ) break;
        auto const tile = cheat_target_square( ss_, ts_ );
        if( !tile.has_value() ) break;
        co_await cheat_create_unit_on_map( ss_, ts_, *nation,
                                           *tile );
        break;
      }
      case e::escape: {
        translated_input_stream_.push( PlayerInput(
            LandViewPlayerInput::exit{}, raw_input.when ) );
        break;
      }
      case e::next_turn: {
        if( !mode_.holds<LandViewMode::end_of_turn>() )
          // We're not supposed to send these events when we're
          // not in EOT mode. However, because of the fact that
          // input events can be buffered, we will be defensive
          // here and check if one of these snuck into the input
          // stream while we were in EOT mode but we now no
          // longer are.
          break;
        translated_input_stream_.push( PlayerInput(
            LandViewPlayerInput::next_turn{}, raw_input.when ) );
        break;
      }
      case e::cmd: {
        translated_input_stream_.push( PlayerInput(
            LandViewPlayerInput::give_command{
              .cmd = raw_input.input.get<LandViewRawInput::cmd>()
                         .what },
            raw_input.when ) );
        break;
      }
      case e::european_status: {
        translated_input_stream_.push(
            PlayerInput( LandViewPlayerInput::european_status{},
                         raw_input.when ) );
        break;
      }
      case e::hidden_terrain: {
        translated_input_stream_.push(
            PlayerInput( LandViewPlayerInput::hidden_terrain{},
                         raw_input.when ) );
        break;
      }
      case e::toggle_view_mode: {
        auto& o = raw_input.input
                      .get<LandViewRawInput::toggle_view_mode>();
        translated_input_stream_.push( PlayerInput(
            LandViewPlayerInput::toggle_view_mode{
              .options = o.options },
            raw_input.when ) );
        break;
      }
      case e::activate: {
        auto& o =
            raw_input.input.get<LandViewRawInput::activate>();
        vector<LandViewPlayerInput> const inputs =
            activate_tile( o.tile );
        // Since we may have just popped open a box to ask the
        // user to select units, just use the "now" time so
        // that these events don't get disgarded. Also, mouse
        // clicks are not likely to get buffered for too long
        // anyway.
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, Clock_t::now() ) );
        break; //
      }
      case e::tile_click: {
        auto& o =
            raw_input.input.get<LandViewRawInput::tile_click>();
        if( o.mods.shf_down ) {
          // cheat mode.
          if( !cheat_mode_enabled( ss_ ) ) break;
          maybe<e_nation> const nation =
              player_for_role( ss_, e_player_role::active );
          if( !nation.has_value() ) break;
          co_await cheat_create_unit_on_map( ss_, ts_, *nation,
                                             o.coord );
          break;
        }
        vector<LandViewPlayerInput> inputs =
            co_await click_on_world_tile( o.coord );
        // Since we may have just popped open a box to ask the
        // user to select units, just use the "now" time so
        // that these events don't get disgarded. Also, mouse
        // clicks are not likely to get buffered for too long
        // anyway.
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, Clock_t::now() ) );
        break;
      }
      case e::tile_right_click: {
        auto& o = raw_input.input
                      .get<LandViewRawInput::tile_right_click>();
        vector<LandViewPlayerInput> inputs =
            co_await right_click_on_world_tile( o.coord );
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, raw_input.when ) );
        break;
      }
      case e::tile_enter: {
        auto& o =
            raw_input.input.get<LandViewRawInput::tile_enter>();
        vector<LandViewPlayerInput> inputs =
            co_await enter_on_world_tile( o.tile );
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, raw_input.when ) );
        break;
      }
      case e::center: {
        // For this one, we just perform the action right here.
        auto const tile = find_tile_to_center_on();
        if( tile.has_value() ) co_await center_on_tile( *tile );
        break;
      }
      case e::context_menu: {
        auto& o = raw_input.input
                      .get<LandViewRawInput::context_menu>();
        co_await context_menu( o.where, o.tile );
        break;
      }
    }
  }

  wait<LandViewPlayerInput> next_player_input_object() {
    while( true ) {
      while( translated_input_stream_.empty() )
        co_await single_raw_input_translator();
      CHECK( !translated_input_stream_.empty() );
      PlayerInput res = translated_input_stream_.front();
      translated_input_stream_.pop();
      // Ignore any input events that are too old.
      if( Clock_t::now() - res.when < chrono::seconds{ 2 } )
        co_return std::move( res.input );
    }
  }

  /****************************************************************
  ** Land View IPlane
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

    if( state( ::SDL_SCANCODE_LCTRL ) ) {
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

  maybe<command> try_orders_from_lua( int keycode,
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
    //   command command = lua_orders.as<command>();
    //
    // And it should do the correct conversion and error
    // checking.
    lua::table tbl = lua_orders.as<lua::table>();
    command command;
    if( false )
      ;
    else if( tbl["wait"] )
      command = command::wait{};
    else if( tbl["forfeight"] )
      command = command::forfeight{};
    else if( tbl["build"] )
      command = command::build{};
    else if( tbl["fortify"] )
      command = command::fortify{};
    else if( tbl["sentry"] )
      command = command::sentry{};
    else if( tbl["disband"] )
      ; // handled in c++
    else if( tbl["road"] )
      command = command::road{};
    else if( tbl["plow"] )
      command = command::plow{};
    else if( tbl["move"] ) {
      e_direction d = tbl["move"]["d"].as<e_direction>();
      command       = command::move{ .d = d };
    } else {
      FATAL(
          "invalid command::move object received from "
          "lua." );
    }
    return command;
  }

  void advance_state() override { advance_viewport_state(); }

  void draw( rr::Renderer& renderer ) const override {
    UNWRAP_CHECK(
        viewport_rect_pixels,
        compositor::section( compositor::e_section::viewport ) );
    LandViewRenderer const lv_renderer(
        ss_, renderer, animator_, viz_, last_unit_input_id(),
        viewport_rect_pixels, input_overrun_indicator_,
        viewport() );

    lv_renderer.render_non_entities();
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
      case e_menu_item::cheat_create_unit_on_map: {
        if( !mode_.holds<LandViewMode::unit_input>() &&
            !mode_.holds<LandViewMode::view_mode>() &&
            !mode_.holds<LandViewMode::end_of_turn>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send( RawInput(
              LandViewRawInput::cheat_create_unit{} ) );
        };
        return handler;
      }
      case e_menu_item::cheat_reveal_map: {
        if( !mode_.holds<LandViewMode::unit_input>() &&
            !mode_.holds<LandViewMode::view_mode>() &&
            !mode_.holds<LandViewMode::end_of_turn>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::reveal_map{} ) );
        };
        return handler;
      }
      case e_menu_item::toggle_view_mode: {
        if( !mode_.holds<LandViewMode::unit_input>() &&
            !mode_.holds<LandViewMode::view_mode>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::toggle_view_mode{} ) );
        };
        return handler;
      }
      case e_menu_item::activate: {
        if( !mode_.holds<LandViewMode::end_of_turn>() &&
            !mode_.holds<LandViewMode::view_mode>() )
          break;
        // This menu item is only expected to be clicked when the
        // white box tile is visible.
        CHECK( white_box_stream_.has_value() );
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::activate{
                .tile = white_box_tile( ss_ ) } ) );
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
        if( !mode_.holds<LandViewMode::unit_input>() ) break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::center{} ) );
        };
        return handler;
      }
      case e_menu_item::sentry: {
        if( !mode_.holds<LandViewMode::unit_input>() ) break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::sentry{} } ) );
        };
        return handler;
      }
      case e_menu_item::fortify: {
        if( !mode_.holds<LandViewMode::unit_input>() ) break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::fortify{} } ) );
        };
        return handler;
      }
      case e_menu_item::disband: {
        if( mode_.holds<LandViewMode::unit_input>() ) {
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::disband{} } ) );
          };
        }
        if( mode_.holds<LandViewMode::view_mode>() ) {
          point const tile = white_box_tile( ss_ );
          if( ss_.units.from_coord( Coord::from_gfx( tile ) )
                  .empty() )
            break;
          return [tile, this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::disband{ .tile = tile } } ) );
          };
        }
        break;
      }
      case e_menu_item::wait: {
        if( mode_.holds<LandViewMode::unit_input>() ) {
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::wait{} } ) );
          };
        }
        break;
      }
      case e_menu_item::build_colony: {
        if( mode_.holds<LandViewMode::unit_input>() ) {
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::build{} } ) );
          };
        }
        break;
      }
      case e_menu_item::return_to_europe: {
        if( auto const unit_input =
                mode_.get_if<LandViewMode::unit_input>() ) {
          // TODO: to implement this like in the OG we need to
          // verify that the unit is a ship and that it is over a
          // sea lane tile. Then there is the question of how to
          // actually make the transition; normally a ship must
          // have an adjacent sea lane square that it can move to
          // (without foreign ships) to go to the harbor, but in
          // the OG it can do it without moving. We need to de-
          // cide if we are going to change this behavior or not.
          // If we do change it then it will avoid "cheating" by
          // making the return-to-harbor more consistent between
          // movement and menu item, but if we don't change it
          // then that would keep things more consistent with the
          // OG in that a ship that can reach at least one sea
          // lane tile won't be blocked from returning to the
          // harbor.
          //
          // Note that in the OG this command does not cause the
          // ship to navigate to a sea lane tile; that action is
          // done via the separate "go to" menu item. As an
          // aside, there is a bug in the OG in that action be-
          // cause a ship needs one less movement point to go to
          // the harbor than it would with manual movement; we
          // will probably fix that.
        }
        break;
      }
      case e_menu_item::dump: {
        if( !mode_.holds<LandViewMode::unit_input>() ) break;
        // Only for things that can carry cargo (ships and wagon
        // trains).
        if( ss_.units
                .unit_for( mode_.get<LandViewMode::unit_input>()
                               .unit_id )
                .desc()
                .cargo_slots == 0 )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::dump{} } ) );
        };
        return handler;
      }
      case e_menu_item::plow: {
        if( !mode_.holds<LandViewMode::unit_input>() ) break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::plow{} } ) );
        };
        return handler;
      }
      case e_menu_item::road: {
        if( !mode_.holds<LandViewMode::unit_input>() ) break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::road{} } ) );
        };
        return handler;
      }
      case e_menu_item::hidden_terrain: {
        if( !mode_.holds<LandViewMode::unit_input>() &&
            !mode_.holds<LandViewMode::view_mode>() &&
            !mode_.holds<LandViewMode::end_of_turn>() )
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
    auto const func = menu_click_handler( item );
    CHECK( func.has_value() );
    ( *func )();
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::unknown_event: //
        break;
      case input::e_input_event::quit_event: //
        break;
      case input::e_input_event::win_event: //
        break;
      case input::e_input_event::key_event: {
        auto& key_event = event.get<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          break;
        handled          = e_input_handled::yes;
        auto is_move_key = [&] {
          return key_event.direction.has_value();
        };

        // NOTE: we need to keep collecting inputs even when the
        // land view state is "none" so that e.g. the user can
        // buffer motion commands for a unit while it is sliding.

        if( mode_.holds<LandViewMode::hidden_terrain>() ) {
          // Here we want to basically allow the user to navigate
          // around with the white box, but any other key should
          // exit hidden terrain mode, regardless of what it
          // would otherwise normally do.
          if( !input::is_mod_key( key_event ) &&
              !is_move_key() && key_event.keycode != ::SDLK_c ) {
            // This will tell hidden-terrain mode to exit.
            raw_input_stream_.send(
                RawInput( LandViewRawInput::hidden_terrain{} ) );
            break;
          }
        }
        // First allow the Lua hook to handle the key press if it
        // wants.
        maybe<command> lua_orders = try_orders_from_lua(
            key_event.keycode, key_event.mod.ctrl_down,
            key_event.mod.shf_down );
        if( lua_orders ) {
          // lg.debug( "received key from lua: {}", lua_orders );
          raw_input_stream_.send( RawInput(
              LandViewRawInput::cmd{ .what = *lua_orders } ) );
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
              bool const center_on_tile =
                  viewport().are_surroundings_visible();
              viewport().smooth_zoom_target( 1.0 );
              if( center_on_tile ) {
                auto const tile = find_tile_to_center_on();
                if( tile.has_value() )
                  viewport().set_point_seek(
                      viewport()
                          .world_tile_to_world_pixel_center(
                              Coord::from_gfx( *tile ) ) );
              }
            }
            break;
          }
          case ::SDLK_F1:
            if( key_event.mod.shf_down ) {
              if( !cheat_mode_enabled( ss_ ) ) break;
              // Cheat mode.
              if( !mode_.holds<LandViewMode::unit_input>() &&
                  !mode_.holds<LandViewMode::end_of_turn>() &&
                  !mode_.holds<LandViewMode::view_mode>() )
                break;
              raw_input_stream_.send( RawInput(
                  LandViewRawInput::cheat_create_unit{} ) );
            }
            break;
          case ::SDLK_v:
            if( key_event.mod.shf_down ) break;
            if( !mode_.holds<LandViewMode::unit_input>() &&
                !mode_.holds<LandViewMode::view_mode>() )
              break;
            raw_input_stream_.send( RawInput(
                LandViewRawInput::toggle_view_mode{} ) );
            break;
          case ::SDLK_w:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::wait{} } ) );
            break;
          case ::SDLK_s:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::sentry{} } ) );
            break;
          case ::SDLK_a:
            if( key_event.mod.shf_down ) break;
            if( !mode_.holds<LandViewMode::end_of_turn>() &&
                !mode_.holds<LandViewMode::view_mode>() )
              break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::activate{
                  .tile = white_box_tile( ss_ ) } ) );
            break;
          case ::SDLK_f:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::fortify{} } ) );
            break;
          case ::SDLK_o:
            // Capital O.
            if( !key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::dump{} } ) );
            break;
          case ::SDLK_b:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::build{} } ) );
            break;
          case ::SDLK_c:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::center{} ) );
            break;
          case ::SDLK_d:
            if( !key_event.mod.shf_down ) break;
            // Note: shift key down.
            if( mode_.holds<LandViewMode::view_mode>() ||
                mode_.holds<LandViewMode::end_of_turn>() )
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::disband{
                      .tile = white_box_tile( ss_ ) } } ) );
            else if( mode_.holds<LandViewMode::unit_input>() )
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::disband{} } ) );
            break;
          case ::SDLK_h:
            if( !key_event.mod.shf_down ) break;
            if( mode_.holds<LandViewMode::hidden_terrain>() )
              // This can happen when pressing shift-h multiple
              // times before the animator gets to the point
              // where it starts taking keys.
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
          case ::SDLK_KP_ENTER:
          case ::SDLK_RETURN:
            if( mode_.holds<LandViewMode::view_mode>() ||
                mode_.holds<LandViewMode::end_of_turn>() ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::tile_enter{
                    .tile = white_box_tile( ss_ ),
                    .mods = key_event.mod } ) );
            }
            break;
          case ::SDLK_SPACE:
          case ::SDLK_KP_5:
            if( mode_.holds<LandViewMode::unit_input>() ) {
              if( key_event.mod.shf_down ) break;
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::forfeight{} } ) );
            } else if( mode_.holds<
                           LandViewMode::end_of_turn>() ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::next_turn{} ) );
            } else if( mode_.holds<LandViewMode::view_mode>() ) {
              raw_input_stream_.send( RawInput(
                  LandViewRawInput::toggle_view_mode{} ) );
            }
            break;
          default:
            if( key_event.mod.shf_down ) break;
            handled = e_input_handled::no;
            if( key_event.direction ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::move{
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
        UNWRAP_BREAK(
            tile,
            viewport().screen_pixel_to_world_tile( val.pos ) );
        handled = e_input_handled::yes;
        lg.debug( "clicked on tile: {}.", tile );
        // Need to only handle "up" events here because if we
        // handled "down" events then that would interfere with
        // dragging.
        switch( val.buttons ) {
          case input::e_mouse_button_event::left_up: {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::tile_click{
                  .coord = tile, .mods = val.mod } ) );
            break;
          }
          case input::e_mouse_button_event::right_up: {
            if( val.mod.shf_down ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::context_menu{
                    .where = val.pos, .tile = tile } ) );
              break;
            }
            raw_input_stream_.send(
                RawInput( LandViewRawInput::tile_right_click{
                  .coord = tile, .mods = val.mod } ) );
            break;
          }
          default:
            break;
        }
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
  bool drag_finished = true;

  wait<> dragging( input::e_mouse_button /*button*/,
                   Coord /*origin*/ ) {
    SCOPE_EXIT { drag_finished = true; };
    while( maybe<DragUpdate> d = co_await drag_stream.next() )
      viewport().pan_by_screen_coords( d->prev - d->current );
  }

  IPlane::e_accept_drag can_drag( input::e_mouse_button button,
                                  Coord origin ) override {
    if( !drag_finished ) return IPlane::e_accept_drag::swallow;
    if( button == input::e_mouse_button::r &&
        viewport().screen_coord_in_viewport( origin ) ) {
      viewport().stop_auto_panning();
      drag_stream.reset();
      drag_finished = false;
      drag_thread   = dragging( button, origin );
      return IPlane::e_accept_drag::yes;
    }
    // Since this is a bottom plane, there is no one else who
    // could handle drag events, so just give them to us as mo-
    // tion events so that we can still generally respond to the
    // mouse movements.
    return IPlane::e_accept_drag::motion;
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
    // lg.debug( "clearing land-view input buffers." );
    raw_input_stream_.reset();
    translated_input_stream_ = {};
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

  wait<> center_on_tile( point const tile ) {
    co_await viewport().center_on_tile_smooth(
        Coord::from_gfx( tile ) );
  }

  void set_visibility( maybe<e_nation> nation ) {
    viz_ = create_visibility_for( ss_, nation );
  }

  void zoom_out_full() {
    viewport().set_zoom( viewport().optimal_min_zoom() );
  }

  maybe<UnitId> unit_blinking() const {
    return mode_.get_if<LandViewMode::unit_input>().member(
        &LandViewMode::unit_input::unit_id );
  }

  maybe<point> white_box() const {
    return white_box_tile_if_visible();
  }

  wait<> eat_cross_unit_buffered_input_events( UnitId id ) {
    if( !config_land_view.input_overrun_detection.enabled )
      co_return;
    // It is necessary to clear the input buffers here because
    // otherwise we could potentially read an old buffered even
    // that was entered longer than kWait ago, which will cause
    // the below to exit the loop prematurely and not eat any-
    // thing.
    reset_input_buffers();
    auto const kWait =
        config_land_view.input_overrun_detection.wait_time;
    SCOPE_EXIT { input_overrun_indicator_ = nothing; };
    int const kMaxInputsToWithold =
        config_land_view.input_overrun_detection
            .max_inputs_to_withold;
    for( int i = 0; i < kMaxInputsToWithold; ++i ) {
      auto const res = co_await co::first(
          raw_input_stream_.next(), wait_for_duration( kWait ) );
      if( res.index() == 1 ) // timeout
        break;
      UNWRAP_CHECK( raw_input, res.get_if<RawInput>() );
      auto command =
          raw_input.input.get_if<LandViewRawInput::cmd>();
      if( !command.has_value() ) {
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
    if( animator_.unit_animations().contains( id ) ) return true;
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
    vector<GenericUnitId> const sorted =
        land_view_unit_stack( ss_, *tile, last_unit_input_id() );
    CHECK( !sorted.empty() );
    return ( sorted[0] == id );
  }

  // Handles interaction while the animation is happening. Con-
  // sume further inputs but eat all of them except for the ones
  // we want, returning only when we should interrupt.
  wait<> hidden_terrain_interact_during_animation() {
    CHECK( mode_.holds<LandViewMode::hidden_terrain>() );
    for( ;; ) {
      RawInput const raw = co_await raw_input_stream_.next();
      SWITCH( raw.input ) {
        CASE( hidden_terrain ) { co_return; }
        CASE( tile_click ) {
          co_await click_on_world_tile( tile_click.coord );
          break;
        }
        default:
          break;
      }
    }
  };

  // Handles interaction while waiting in hidden_terrain mode.
  wait<LandViewPlayerInput> hidden_terrain_white_box_loop(
      point const initial_tile ) {
    wait<> const _ = white_box_thread( initial_tile );
    while( true ) {
      auto const input = co_await white_box_input();
      if( input->get_if<LandViewPlayerInput::hidden_terrain>() )
        co_return *input;
    }
  }

  wait<> show_hidden_terrain() {
    auto const new_state = LandViewMode::hidden_terrain{};
    // Clear input buffers after changing state to the new state
    // and after switching back to the old state.
    SCOPE_EXIT { reset_input_buffers(); };
    SCOPED_SET_AND_RESTORE( mode_, new_state );
    reset_input_buffers();
    // TODO: maybe we could find some way of animating the re-
    // moval of the fog instead of just having it disappear.
    auto const defogger =
        ts_.map_updater().push_options_and_redraw(
            []( MapUpdaterOptions& options ) {
              options.render_fog_of_war = false;
            } );

    HiddenTerrainAnimationSequence const seq =
        anim_seq_for_hidden_terrain( ss_, *viz_, ts_.rand );

    auto const tile = find_a_good_white_box_location(
        ss_, last_unit_input_id(),
        ss_.land_view.viewport.covered_tiles() );

    co_await co::first(
        animator_.animate_sequence( seq.hide ),
        hidden_terrain_interact_during_animation() );

    co_await co::first(
        animator_.animate_sequence_and_hold( seq.hold ),
        hidden_terrain_white_box_loop( tile ) );

    co_await co::first(
        animator_.animate_sequence( seq.show ),
        hidden_terrain_interact_during_animation() );
  }

  wait<> white_box_thread( point const initial ) {
    CHECK( !white_box_stream_.has_value() );
    auto& stream = white_box_stream_.emplace();
    SCOPE_EXIT { white_box_stream_.reset(); };
    set_white_box_tile( ss_, initial );
    while( true ) {
      point const tile     = white_box_tile( ss_ );
      wait<> const blinker = animator_.animate_white_box();
      wait<> const panner  = animator_.ensure_visible( tile );
      set_white_box_tile( ss_, co_await stream.next() );
    }
  }

  wait<maybe<LandViewPlayerInput>> white_box_input() {
    LandViewPlayerInput const input =
        co_await next_player_input_object();

    auto const direction =
        input.inner_if<LandViewPlayerInput::give_command>()
            .inner_if<command::move>();

    if( !direction.has_value() ) co_return input;

    white_box_stream_->send(
        white_box_tile( ss_ )
            .moved( *direction )
            .clamped(
                viz_->rect_tiles().to_gfx().with_dec_size() ) );

    co_return nothing;
  }

  wait<LandViewPlayerInput> white_box_input_loop(
      point const initial_tile ) {
    wait<> const _ = white_box_thread( initial_tile );
    // This loop typically returns on the first iteration in
    // order to deliver the command to the caller, but it may
    // loop if the player is just doing things that can be han-
    // dled in this module, such as moving the white box tile or
    // doing things that only affect the land view display.
    while( true ) {
      auto const input = co_await white_box_input();
      if( input.has_value() ) co_return *input;
    }
  }

  wait<LandViewPlayerInput> show_view_mode(
      ViewModeOptions const options ) {
    auto const new_state = LandViewMode::view_mode{};
    // Clear input buffers after changing state to the new state
    // and after switching back to the old state.
    SCOPE_EXIT { reset_input_buffers(); };
    SCOPED_SET_AND_RESTORE( mode_, new_state );
    reset_input_buffers();
    point const initial_tile =
        options.initial_tile.has_value()
            ? *options.initial_tile
            : find_a_good_white_box_location(
                  ss_, last_unit_input_id(),
                  ss_.land_view.viewport.covered_tiles() );
    // Now that we've extracted potential info from this, reset
    // it since the fact that we're changing modes means that we
    // don't want the continuity behavior when we exit this mode
    // and return to asking order for the unit which would other-
    // wise be enabled by letting this remain set.
    last_unit_input_ = nothing;
    co_return co_await white_box_input_loop( initial_tile );
  }

  wait<LandViewPlayerInput> get_next_input( UnitId id ) {
    // If the last unit that asked for orders no longer exists
    // then clear all state associated with it.
    if( !last_unit_input_id() ) last_unit_input_ = nothing;

    // We only pan to the unit here because if we did that out-
    // side of this if statement then the viewport would pan to
    // the blinking unit after the player e.g. clicks on another
    // unit to activate it.
    if( last_unit_input_id() != id )
      g_needs_scroll_to_unit_on_input = true;

    // Input buffers. If there was no previous unit asking for
    // orders (or if that unit no longer exists) then definitely
    // clear the buffers.
    if( !last_unit_input_.has_value() ) reset_input_buffers();

    // The idea of this is that if we're starting on a new unit
    // or if we're still on the same unit but a window popped up
    // since it last asked for orders (which could have happened
    // as a result of its move or other processing during the
    // turn) then we want to clear the input buffers. Otherwise,
    // it could create a strange situation where the player ob-
    // serves buffered input events processed after a window
    // closes.
    if( last_unit_input_.has_value() &&
        ts_.gui.total_windows_created() >
            last_unit_input_->window_count ) {
      // This means that a window popped up since the last time
      // that a unit was asking for orders. In that case we prob-
      // ably don't need the input buffer shield because the pre-
      // vious unit's movement has been interrupted by a window.
      // And for the same reason we should clear buffered input
      // events.
      last_unit_input_->window_count =
          ts_.gui.total_windows_created();
      last_unit_input_->need_input_buffer_shield = false;
      reset_input_buffers();
    }

    // This might be true either because we started a new turn,
    // or because of the above assignment.
    if( g_needs_scroll_to_unit_on_input )
      co_await animator_.ensure_visible_unit( id );
    g_needs_scroll_to_unit_on_input = false;

    // Need to set this before taking any user input, otherwise
    // we will be in the "none" state, which represents the
    // end-of-turn, in which case e.g. a space bar gets sent as a
    // "next turn" event as opposed to a "forfeight" event (which
    // can happen because sometimes we take input while eating
    // buffered input events below).
    SCOPED_SET_AND_RESTORE(
        mode_, LandViewMode::unit_input{ .unit_id = id } );

    // When we start on a new unit clear the input queue so that
    // commands that were accidentally buffered while controlling
    // the previous unit don't affect this new one, which would
    // almost certainly not be desirable. This function does that
    // clearing in an interactive way in order to signal to the
    // user what is happening.
    bool const eat_buffered =
        last_unit_input_id().has_value() &&
        // If still moving same unit then no need to shield.
        *last_unit_input_id() != id &&
        last_unit_input_->need_input_buffer_shield &&
        // E.g. if the previous unit got on a ship then the
        // player doesn't expect them to continue moving, so no
        // need to shield this unit.
        is_unit_on_map( ss_.units, *last_unit_input_id() );
    if( eat_buffered ) {
      // We typically wait for a bit while eating input events;
      // during that time, make sure that the unit who is about
      // to ask for orders is rendered on the front.
      AnimationSequence const seq = anim_seq_unit_to_front( id );
      // We need the "hold" version because we need this to keep
      // going until the buffer eating is complete, at which
      // point it will be cancelled.
      wait<> anim = animator_.animate_sequence_and_hold( seq );
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

    last_unit_input_.emplace( ss_, /*unit_id=*/id );
    last_unit_input_->need_input_buffer_shield = true;
    last_unit_input_->window_count =
        ts_.gui.total_windows_created();

    // Run the blinker while waiting for user input. The question
    // is, do we want the blinking to start "on" or "off"? The
    // idea is that we want to start it off in the opposite vi-
    // sual state that the unit currently is in.
    LandViewPlayerInput input = co_await co::background(
        next_player_input_object(),
        animator_.animate_blink( id, visible_initially ) );

    // Disable input buffer shielding when the unit issues a
    // non-move command, because in that case it is unlikely that
    // the player will assume that the subsequent command they
    // issue would go to the same unit. E.g., if the unit issues
    // a command to fortify this unit or to open a colony, then
    // they know that the next command they issue would go to the
    // new unit, so we don't need to shield it.
    maybe<command::move const&> move_cmd =
        input.get_if<LandViewPlayerInput::give_command>()
            .member( &LandViewPlayerInput::give_command::cmd )
            .get_if<command::move>();
    if( last_unit_input_.has_value() && !move_cmd.has_value() )
      last_unit_input_->need_input_buffer_shield = false;

    co_return input;
  }

  wait<LandViewPlayerInput> eot_get_next_input() {
    last_unit_input_ = nothing;
    SCOPED_SET_AND_RESTORE( mode_, LandViewMode::end_of_turn{} );
    point const initial_tile = find_a_good_white_box_location(
        ss_, /*last_unit_input=*/nothing,
        ss_.land_view.viewport.covered_tiles() );
    co_return co_await white_box_input_loop( initial_tile );
  }

  void on_logical_resolution_changed( e_resolution ) override {}
};

/****************************************************************
** LandViewPlane
*****************************************************************/
IPlane& LandViewPlane::impl() { return *impl_; }

LandViewPlane::~LandViewPlane() = default;

LandViewPlane::LandViewPlane( SS& ss, TS& ts,
                              maybe<e_nation> visibility )
  : impl_( new Impl( ss, ts, visibility ) ) {}

wait<> LandViewPlane::ensure_visible( Coord const& coord ) {
  return impl_->animator_.ensure_visible( coord.to_gfx() );
}

wait<> LandViewPlane::center_on_tile( point const tile ) {
  return impl_->center_on_tile( tile );
}

void LandViewPlane::set_visibility( maybe<e_nation> nation ) {
  return impl_->set_visibility( nation );
}

wait<> LandViewPlane::ensure_visible_unit( GenericUnitId id ) {
  return impl_->animator_.ensure_visible_unit( id );
}

wait<> LandViewPlane::show_hidden_terrain() {
  return impl_->show_hidden_terrain();
}

wait<LandViewPlayerInput> LandViewPlane::show_view_mode(
    ViewModeOptions const options ) {
  return impl_->show_view_mode( options );
}

wait<LandViewPlayerInput> LandViewPlane::get_next_input(
    UnitId id ) {
  return impl_->get_next_input( id );
}

wait<LandViewPlayerInput> LandViewPlane::eot_get_next_input() {
  return impl_->eot_get_next_input();
}

void LandViewPlane::start_new_turn() {
  return impl_->start_new_turn();
}

void LandViewPlane::zoom_out_full() {
  return impl_->zoom_out_full();
}

maybe<UnitId> LandViewPlane::unit_blinking() const {
  return impl_->unit_blinking();
}

maybe<point> LandViewPlane::white_box() const {
  return impl_->white_box();
}

wait<> LandViewPlane::animate( AnimationSequence const& seq ) {
  return impl_->animator_.animate_sequence( seq );
}

} // namespace rn
