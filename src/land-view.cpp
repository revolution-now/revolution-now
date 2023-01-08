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

// Revolution Now
#include "anim.hpp"
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "co-time.hpp"
#include "co-wait.hpp"
#include "colony-id.hpp"
#include "compositor.hpp"
#include "imap-updater.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "orders.hpp"
#include "physics.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "render.hpp"
#include "road.hpp"
#include "screen.hpp"
#include "sound.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "time.hpp"
#include "ts.hpp"
#include "unit-id.hpp"
#include "unit-stack.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "visibility.hpp"
#include "window.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/natives.hpp"
#include "config/rn.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/land-view.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/scope-exit.hpp"

#ifdef L
#  undef L
#endif

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// Rds
#include "land-view-impl.rds.hpp"

// C++ standard library
#include <chrono>
#include <queue>
#include <unordered_map>

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

/****************************************************************
** Land-View Rendering
*****************************************************************/
struct LandViewRenderer {
  rr::Renderer& renderer;
  Rect const    covered = {};
};

// This will render the background around the zoomed-out map.
// This background consists of some giant stretched ocean tiles.
// It goes like this:
//
//   1. The tiles are scaled up, but only as large as possible so
//      that they can remain as squares; so the tile size will be
//      equal to the shorter side length of the viewport.
//   2. Those tiles are then tiled to cover all of the area.
//   3. Steps 1+2 are repeated two more times with partial alpha
//      (i.e., layered on top of the previous), but each time
//      being scaled up slightly more. The scaling is done about
//      the center of the composite image in order create a
//      "zooming" effect. To achieve this, the composite (total,
//      tiled) image is rendered around the origin and the GPU
//      then scales it and then translates it.
//
// As mentioned, all of the layers are done with partial alpha so
// that they all end up visible and thus create a "zooming" ef-
// fect.
void render_backdrop( rr::Renderer& renderer ) {
  SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, 0.4 );
  UNWRAP_CHECK(
      viewport_rect_pixels,
      compositor::section( compositor::e_section::viewport ) );
  auto const [shortest_side, longest_side] = [&] {
    Delta const delta         = viewport_rect_pixels.delta();
    int         shortest_side = std::min( delta.w, delta.h );
    int         longest_side  = std::max( delta.w, delta.h );
    return pair{ shortest_side, longest_side };
  }();
  int const num_squares_needed =
      longest_side / shortest_side + 1;
  Delta const tile_size =
      Delta{ .w = W{ shortest_side }, .h = H{ shortest_side } };
  Rect const tiled_rect =
      Rect::from( Coord{},
                  tile_size * Delta{ .w = num_squares_needed,
                                     .h = num_squares_needed } )
          .centered_on( Coord{} );
  Delta const shift = viewport_rect_pixels.center() -
                      viewport_rect_pixels.upper_left();
  double       scale      = 1.00;
  double const kScaleInc  = .014;
  int const    kNumLayers = 4;
  for( int i = 0; i < kNumLayers; ++i ) {
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, scale );
    SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                             gfx::size( shift ).to_double() );
    rr::Painter painter = renderer.painter();
    for( Rect rect : gfx::subrects( tiled_rect, tile_size ) )
      render_sprite( painter,
                     Rect::from( rect.upper_left(), tile_size ),
                     e_tile::terrain_ocean );
    scale += kScaleInc;
  }
}

} // namespace

struct LandViewPlane::Impl : public Plane {
  Planes&    planes_;
  SS&        ss_;
  TS&        ts_;
  Visibility viz_;

  vector<MenuPlane::Deregistrar> dereg;

  co::stream<RawInput>    raw_input_stream_;
  co::stream<PlayerInput> translated_input_stream_;
  unordered_map<GenericUnitId, UnitAnimation_t> unit_animations_;
  unordered_map<ColonyId, ColonyAnimation_t> colony_animations_;
  unordered_map<DwellingId, DwellingAnimation_t>
                 dwelling_animations_;
  LandViewMode_t landview_mode_ = LandViewMode::none{};

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

  // A fading hourglass icon will be drawn on this unit to signal
  // to the player that the movement command just entered will be
  // thrown out in order to avoid inadvertantly giving the new
  // unit an order intended for the old unit.
  struct InputOverrunIndicator {
    UnitId unit_id    = {};
    Time_t start_time = {};
  };
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
      viz_( Visibility::create( ss, nation ) ) {
    register_menu_items( planes.menu() );
    // Initialize general global data.
    unit_animations_.clear();
    colony_animations_.clear();
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

  wait<> animate_depixelation( GenericUnitId id,
                               maybe<e_tile> target_tile ) {
    CHECK( !unit_animations_.contains( id ) );
    UnitAnimation::depixelate_unit& depixelate =
        unit_animations_[id]
            .emplace<UnitAnimation::depixelate_unit>();
    SCOPE_EXIT( unit_animations_.erase( id ) );
    depixelate.stage  = 0.0;
    depixelate.target = target_tile;

    AnimThrottler throttle( kAlmostStandardFrame );
    while( depixelate.stage <= 1.0 ) {
      co_await throttle();
      depixelate.stage += config_rn.depixelate_per_frame;
    }
  }

  wait<> animate_colony_depixelation_impl(
      Colony const& colony ) {
    CHECK( !colony_animations_.contains( colony.id ) );
    ColonyAnimation::depixelate& depixelate =
        colony_animations_[colony.id]
            .emplace<ColonyAnimation::depixelate>();
    SCOPE_EXIT( colony_animations_.erase( colony.id ) );
    depixelate.stage = 0.0;

    AnimThrottler throttle( kAlmostStandardFrame );
    while( depixelate.stage <= 1.0 ) {
      co_await throttle();
      depixelate.stage += config_rn.depixelate_per_frame;
    }
  }

  // TODO: this animation needs to be sync'd with the one in the
  // mini-map.
  wait<> animate_blink( UnitId id, bool visible_initially ) {
    using namespace std::literals::chrono_literals;
    CHECK( !unit_animations_.contains( id ) );
    UnitAnimation::blink& blink =
        unit_animations_[id].emplace<UnitAnimation::blink>();
    blink = UnitAnimation::blink{ .visible = visible_initially };
    SCOPE_EXIT( unit_animations_.erase( id ) );
    // We use an initial delay so that our initial value of `vis-
    // ible` will be the first to linger.
    AnimThrottler throttle( 500ms, /*initial_delay=*/true );
    while( true ) {
      co_await throttle();
      blink.visible = !blink.visible;
    }
  }

  wait<> animate_slide( GenericUnitId id, e_direction d ) {
    CHECK( !unit_animations_.contains( id ) );
    UnitAnimation::slide& mv =
        unit_animations_[id].emplace<UnitAnimation::slide>();
    SCOPE_EXIT( unit_animations_.erase( id ) );

    // TODO: make this a game option.
    double const kMaxVelocity =
        ss_.settings.fast_piece_slide ? .1 : .07;

    mv = UnitAnimation::slide{
        .direction   = d,
        .percent     = 0.0,
        .percent_vel = DissipativeVelocity{
            /*min_velocity=*/0,            //
            /*max_velocity=*/kMaxVelocity, //
            /*initial_velocity=*/.1,       //
            /*mag_acceleration=*/1,        //
            /*mag_drag_acceleration=*/.002 //
        }                                  //
    };
    AnimThrottler throttle( kAlmostStandardFrame );
    while( mv.percent <= 1.0 ) {
      co_await throttle();
      mv.percent_vel.advance( e_push_direction::none );
      mv.percent += mv.percent_vel.to_double();
    }
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

  /**************************************************************
   * Unit Rendering.
   **************************************************************/
  // `multiple_units` is for the case when there are multiple
  // units on the square and we want to indicate that visually.
  void render_single_unit( rr::Renderer& renderer, Coord where,
                           GenericUnitId id,
                           e_flag_count  flag_count ) const {
    switch( ss_.units.unit_kind( id ) ) {
      case e_unit_kind::euro: {
        UnitId const unit_id{ to_underlying( id ) };
        render_unit(
            renderer, where, ss_.units.unit_for( unit_id ),
            UnitRenderOptions{ .flag   = flag_count,
                               .shadow = UnitShadow{} } );
        break;
      }
      case e_unit_kind::native: {
        NativeUnitId const unit_id{ to_underlying( id ) };
        render_native_unit(
            renderer, where, ss_, ss_.units.unit_for( unit_id ),
            UnitRenderOptions{ .flag   = flag_count,
                               .shadow = UnitShadow{} } );
        break;
      }
    }
  }

  // `multiple_units` is for the case when there are multiple
  // units on the square and we want to indicate that visually.
  void render_single_unit_depixelate_to(
      rr::Renderer& renderer, Coord where, GenericUnitId id,
      bool multiple_units, double stage,
      e_tile target_tile ) const {
    e_flag_count const flag_style = multiple_units
                                        ? e_flag_count::multiple
                                        : e_flag_count::single;
    switch( ss_.units.unit_kind( id ) ) {
      case e_unit_kind::euro:
        render_unit_depixelate_to(
            renderer, where, ss_, ss_.units.euro_unit_for( id ),
            target_tile, stage,
            UnitRenderOptions{ .flag   = flag_style,
                               .shadow = UnitShadow{} } );
        break;
      case e_unit_kind::native:
        render_native_unit_depixelate_to(
            renderer, where, ss_,
            ss_.units.native_unit_for( id ), target_tile, stage,
            UnitRenderOptions{ .flag   = flag_style,
                               .shadow = UnitShadow{} } );
        break;
    }
  }

  // Given a tile, compute the screen rect where it should be
  // rendered.
  Rect render_rect_for_tile( Rect covered, Coord tile ) const {
    Delta delta_in_tiles  = tile - covered.upper_left();
    Delta delta_in_pixels = delta_in_tiles * g_tile_delta;
    return Rect::from( Coord{} + delta_in_pixels, g_tile_delta );
  }

  // Given a tile, this will return the ordered unit stack on
  // that tile (if any units are present), in the order that they
  // would be rendered. The top-most unit is first.
  vector<GenericUnitId> unit_stack( Coord tile ) const {
    unordered_set<GenericUnitId> const& units =
        ss_.units.from_coord( tile );
    vector<GenericUnitId> res;
    if( units.empty() ) return res;
    res = vector<GenericUnitId>( units.begin(), units.end() );
    maybe<GenericUnitId> const last_unit_id =
        last_unit_input_.member( &LastUnitInput::unit_id );
    sort_unit_stack( ss_, res );
    return res;
  }

  void render_units_on_square( rr::Renderer& renderer,
                               Rect covered, Coord tile,
                               bool flags ) const {
    if( !viz_.visible( tile ) ) return;
    maybe<GenericUnitId> const last_unit_id =
        last_unit_input_.member( &LastUnitInput::unit_id );
    // This will be sorted in decreasing order of defense, then
    // by decreasing id.
    vector<GenericUnitId> sorted = unit_stack( tile );
    if( sorted.empty() ) return;
    erase_if( sorted, [this]( GenericUnitId id ) {
      return unit_animations_.contains( id );
    } );
    // This is optional, but always put the most recent unit to
    // ask for orders at the top of the stack if they are in this
    // tile. This makes for a better UX since e.g. if a unit is
    // on a square with other units and it attempts to make an
    // invalid move, it will remain on top while the message box
    // pops up with the explanation.
    if( last_unit_id.has_value() &&
        find( sorted.begin(), sorted.end(), *last_unit_id ) !=
            sorted.end() ) {
      erase( sorted, *last_unit_id );
      sorted.insert( sorted.begin(), *last_unit_id );
    }
    if( sorted.empty() ) return;
    GenericUnitId const max_defense = sorted[0];

    Coord const where =
        render_rect_for_tile( covered, tile ).upper_left();
    bool const         multiple_units = ( sorted.size() > 1 );
    e_flag_count const flag_count = !flags ? e_flag_count::none
                                    : !multiple_units
                                        ? e_flag_count::single
                                        : e_flag_count::multiple;
    render_single_unit( renderer, where, max_defense,
                        flag_count );
  }

  vector<pair<Coord, GenericUnitId>> units_to_render(
      Rect covered ) const {
    // This is for efficiency. When we are sufficiently zoomed
    // out then it is more efficient to iterate over units then
    // covered tiles, whereas the reverse is true when zoomed in.
    unordered_map<GenericUnitId, UnitState_t> const& all =
        ss_.units.all();
    int const num_units = all.size();
    int const num_tiles = covered.area();
    vector<pair<Coord, GenericUnitId>> res;
    res.reserve( num_units );
    if( num_tiles > num_units ) {
      // Iterate over units.
      for( auto const& [id, state] : all )
        if( maybe<Coord> coord =
                coord_for_unit_indirect( ss_.units, id );
            coord.has_value() && coord->is_inside( covered ) )
          res.emplace_back( *coord, id );
    } else {
      // Iterate over covered tiles.
      for( Rect tile : gfx::subrects( covered ) )
        for( GenericUnitId generic_id :
             ss_.units.from_coord( tile.upper_left() ) )
          res.emplace_back( tile.upper_left(), generic_id );
    }
    return res;
  }

  void render_units_default( rr::Renderer& renderer,
                             Rect          covered ) const {
    unordered_set<Coord> hit;
    for( auto [tile, id] : units_to_render( covered ) ) {
      if( ss_.colonies.maybe_from_coord( tile ).has_value() )
        continue;
      if( hit.contains( tile ) ) continue;
      render_units_on_square( renderer, covered, tile,
                              /*flags=*/true );
      hit.insert( tile );
    }
  }

  void render_units_impl( rr::Renderer& renderer,
                          Rect          covered ) const {
    if( unit_animations_.empty() )
      return render_units_default( renderer, covered );
    // We have some units being animated, so now things get com-
    // plicated. This will be the case most of the time, since
    // there is usually at least one unit blinking. The exception
    // would be the end-of-turn when there should be no anima-
    // tions.

    unordered_map<GenericUnitId, UnitAnimation::front const*>
        front;
    unordered_map<GenericUnitId, UnitAnimation::blink const*>
        blink;
    unordered_map<GenericUnitId, UnitAnimation::slide const*>
        slide;
    unordered_map<GenericUnitId,
                  UnitAnimation::depixelate_unit const*>
        depixelate_unit;
    // These are the tiles to skip when rendering units that are
    // not animated. An example would be that if a unit is
    // blinking then we don't want to render any other units on
    // that tile.
    unordered_set<Coord> tiles_to_skip;
    // These are tiles where we want to draw units but faded and
    // with no flags so that the unit in front will be more dis-
    // cernible but the player will still see that there are
    // units behind.
    unordered_set<Coord> tiles_to_fade;
    for( auto const& [id, anim] : unit_animations_ ) {
      Coord const tile =
          coord_for_unit_indirect_or_die( ss_.units, id );
      switch( anim.to_enum() ) {
        case UnitAnimation::e::front:
          front[id] = &anim.get<UnitAnimation::front>();
          tiles_to_skip.insert( tile );
          break;
        case UnitAnimation::e::blink:
          blink[id] = &anim.get<UnitAnimation::blink>();
          tiles_to_skip.insert( tile );
          break;
        case UnitAnimation::e::slide:
          slide[id] = &anim.get<UnitAnimation::slide>();
          tiles_to_fade.insert( tile );
          break;
        case UnitAnimation::e::depixelate_unit:
          depixelate_unit[id] =
              &anim.get<UnitAnimation::depixelate_unit>();
          tiles_to_skip.insert( tile );
          break;
      }
    }

    // Render all non-animated units except for those on tiles
    // that we want to skip.
    unordered_set<Coord> hit;
    for( auto [tile, id] : units_to_render( covered ) ) {
      if( tiles_to_skip.contains( tile ) ) continue;
      if( ss_.colonies.maybe_from_coord( tile ).has_value() )
        continue;
      if( unit_animations_.contains( id ) ) continue;
      if( hit.contains( tile ) ) continue;
      hit.insert( tile );
      if( tiles_to_fade.contains( tile ) ) {
        SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .25 );
        render_units_on_square( renderer, covered, tile,
                                /*flags=*/false );
      } else {
        render_units_on_square( renderer, covered, tile,
                                /*flags=*/true );
      }
    }

    auto render_impl = [&]( GenericUnitId id, auto const& f ) {
      Coord const tile =
          coord_for_unit_multi_ownership_or_die( ss_, id );
      if( !viz_.visible( tile ) ) return;
      Coord const where =
          render_rect_for_tile( covered, tile ).upper_left();
      bool const multiple_units =
          ss_.units.from_coord( tile ).size() > 1;
      e_flag_count const flag_count =
          multiple_units ? e_flag_count::multiple
                         : e_flag_count::single;
      f( where, flag_count );
    };

    // 1. Render units that are supposed to hover above a colony.
    for( auto const& [id, anim] : front ) {
      render_impl( id, [&]( Coord        where,
                            e_flag_count flag_count ) {
        render_single_unit( renderer, where, id, flag_count );
      } );
    }

    // 2. Render units that are blinking.
    for( auto const& [id, anim] : blink ) {
      if( !anim->visible ) continue;
      render_impl( id, [&]( Coord where, e_flag_count ) {
        render_single_unit( renderer, where, id,
                            e_flag_count::single );
      } );
    }

    // 3. Render units that are sliding.
    for( auto const& [id, anim] : slide ) {
      Coord const mover_coord =
          coord_for_unit_indirect_or_die( ss_.units, id );
      // Now render the sliding unit.
      Delta const pixel_delta =
          ( ( mover_coord.moved( anim->direction ) -
              mover_coord ) *
            g_tile_delta )
              .multiply_and_round( anim->percent );
      render_impl( id, [&]( Coord where, e_flag_count ) {
        render_single_unit( renderer, where + pixel_delta, id,
                            e_flag_count::single );
      } );
    }

    // 4. Render units that are depixelating.
    for( auto const& [id, anim] : depixelate_unit ) {
      // Check if we are depixelating to another unit.
      if( !anim->target.has_value() ) {
        Coord const tile =
            coord_for_unit_multi_ownership_or_die( ss_, id );
        Coord const loc =
            render_rect_for_tile( covered, tile ).upper_left();
        // Render and depixelate both the unit and the flag.
        SCOPED_RENDERER_MOD_SET(
            painter_mods.depixelate.hash_anchor, loc );
        SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                                 anim->stage );
        render_impl( id, [&]( Coord where, e_flag_count ) {
          render_single_unit( renderer, where, id,
                              e_flag_count::single );
        } );
      } else {
        CHECK( anim->target.has_value() );
        // Render and the unit and the flag but only depixelate
        // the unit to the target unit. This function will set
        // the hash anchor and stage ultimately.
        render_impl( id, [&]( Coord where, e_flag_count ) {
          render_single_unit_depixelate_to(
              renderer, where, id, /*multiple_units=*/false,
              anim->stage, *anim->target );
        } );
      }
    }
  }

  void render_native_dwelling( rr::Painter&    painter,
                               Rect            covered,
                               Dwelling const& dwelling ) const {
    if( !viz_.visible( dwelling.location ) ) return;
    Coord tile_coord =
        render_rect_for_tile( covered, dwelling.location )
            .upper_left() -
        Delta{ .w = 6, .h = 6 };
    render_dwelling( painter, tile_coord, dwelling );
  }

  void render_native_dwelling_depixelate(
      rr::Renderer& renderer, Rect covered,
      Dwelling const& dwelling ) const {
    UNWRAP_CHECK(
        animation,
        base::lookup( dwelling_animations_, dwelling.id )
            .get_if<DwellingAnimation::depixelate>() );
    // As usual, the hash anchor coord is arbitrary so long as
    // its position is fixed relative to the sprite.
    Coord const hash_anchor =
        render_rect_for_tile( covered, dwelling.location )
            .upper_left();
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             animation.stage );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                             hash_anchor );
    rr::Painter painter = renderer.painter();
    render_native_dwelling( painter, covered, dwelling );
  }

  void render_native_dwellings( rr::Renderer& renderer,
                                Rect          covered ) const {
    rr::Painter painter = renderer.painter();
    unordered_map<DwellingId, Dwelling> const& all =
        ss_.natives.dwellings_all();
    switch( landview_mode_.to_enum() ) {
      using namespace LandViewMode;
      case e::dwelling_disappearing: {
        auto& o =
            landview_mode_
                .get<LandViewMode::dwelling_disappearing>();
        DwellingId const disappearing_id = o.dwelling_id;
        for( auto const& [id, dwelling] : all ) {
          if( dwelling.location.is_inside( covered ) ) {
            if( id == disappearing_id )
              render_native_dwelling_depixelate(
                  renderer, covered, dwelling );
            else
              render_native_dwelling( painter, covered,
                                      dwelling );
          }
        }
        break;
      }
      default: {
        for( auto const& [id, dwelling] : all )
          if( dwelling.location.is_inside( covered ) )
            render_native_dwelling( painter, covered, dwelling );
        break;
      }
    }
  }

  void render_colony( rr::Renderer& renderer,
                      rr::Painter& painter, Rect covered,
                      Colony const& colony ) const {
    if( !viz_.visible( colony.location ) ) return;
    Coord tile_coord =
        render_rect_for_tile( covered, colony.location )
            .upper_left();
    Coord colony_sprite_upper_left =
        tile_coord - Delta{ .w = 6, .h = 6 };
    rn::render_colony( painter, colony_sprite_upper_left,
                       colony );
    Coord name_coord =
        tile_coord + config_land_view.colony_name_offset;
    render_text_markup(
        renderer, name_coord, config_land_view.colony_name_font,
        TextMarkupInfo{
            .shadowed_text_color   = gfx::pixel::white(),
            .shadowed_shadow_color = gfx::pixel::black() },
        fmt::format( "@[S]{}@[]", colony.name ) );
  }

  void render_colony_depixelate( rr::Renderer& renderer,
                                 Rect          covered,
                                 Colony const& colony ) const {
    UNWRAP_CHECK( animation,
                  base::lookup( colony_animations_, colony.id )
                      .get_if<ColonyAnimation::depixelate>() );
    // As usual, the hash anchor coord is arbitrary so long as
    // its position is fixed relative to the sprite.
    Coord const hash_anchor =
        render_rect_for_tile( covered, colony.location )
            .upper_left();
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             animation.stage );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                             hash_anchor );
    rr::Painter painter = renderer.painter();
    render_colony( renderer, painter, covered, colony );
  }

  void render_colonies( rr::Renderer& renderer,
                        Rect          covered ) const {
    rr::Painter painter = renderer.painter();
    // FIXME: since colony icons spill over the usual 32x32 tile
    // we need to render colonies that are beyond the `covered`
    // rect.
    unordered_map<ColonyId, Colony> const& all =
        ss_.colonies.all();
    switch( landview_mode_.to_enum() ) {
      using namespace LandViewMode;
      case e::colony_disappearing: {
        auto& o = landview_mode_
                      .get<LandViewMode::colony_disappearing>();
        ColonyId disappearing_id = o.colony_id;
        for( auto const& [id, colony] : all ) {
          if( colony.location.is_inside( covered ) ) {
            if( id == disappearing_id )
              render_colony_depixelate( renderer, covered,
                                        colony );
            else
              render_colony( renderer, painter, covered,
                             colony );
          }
        }
        break;
      }
      default: {
        for( auto const& [id, colony] : all )
          if( colony.location.is_inside( covered ) )
            render_colony( renderer, painter, covered, colony );
        break;
      }
    }
  }

  void render_units_under_colonies( rr::Renderer& renderer,
                                    Rect covered ) const {
    // Currently the only use case for rendering a unit under a
    // colony is when the colony is depixelating and we want to
    // reveal any units that are there.
    auto o = landview_mode_
                 .get_if<LandViewMode::colony_disappearing>();
    if( !o.has_value() ) return;
    Coord const location =
        ss_.colonies.colony_for( o->colony_id ).location;
    if( !location.is_inside( covered ) ) return;
    render_units_on_square( renderer, covered, location,
                            /*flags=*/false );
  }

  // When the player is moving a unit and it runs out of movement
  // points there is a chance that the player will accidentally
  // issue a couple of extra input commands to the unit beyond
  // the end of its turn. If that happens then the very next unit
  // to ask for orders would get those commands and move in a way
  // that the player likely had not intended. So we have a mecha-
  // nism (logic elsewhere) of preventing that, and the visual
  // indicator is an hourglass temporarily drawn on the new unit
  // let the player know that a few input commands were thrown
  // out in order to avoid inadvertantly giving them to the new
  // unit.
  //
  // TODO: rendering this is technically optional. Decide whether
  // to keep it. If this ends up being removed then the config
  // options referenced can also be removed.
  void render_input_overrun_indicator( rr::Renderer& renderer,
                                       Rect covered ) const {
    if( !input_overrun_indicator_.has_value() ) return;
    InputOverrunIndicator const& indicator =
        *input_overrun_indicator_;
    maybe<Coord> const unit_coord =
        coord_for_unit_indirect( ss_.units, indicator.unit_id );
    if( !unit_coord.has_value() ) return;
    if( !unit_coord->is_inside( covered.with_border_added() ) )
      return;
    Rect const indicator_render_rect =
        render_rect_for_tile( covered, *unit_coord );
    auto const kHoldTime =
        config_land_view.input_overrun_detection
            .hourglass_hold_time;
    auto const kFadeTime =
        config_land_view.input_overrun_detection
            .hourglass_fade_time;
    double     alpha = 1.0;
    auto const delta = Clock_t::now() - indicator.start_time;
    if( delta > kHoldTime ) {
      auto fade_time =
          clamp( duration_cast<chrono::milliseconds>(
                     delta - kHoldTime ),
                 0ms, kFadeTime );
      alpha = double( fade_time.count() ) / kFadeTime.count();
      alpha = 1.0 - alpha;
    }
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
    rr::Painter painter = renderer.painter();
    render_sprite( painter, indicator_render_rect.upper_left(),
                   e_tile::lift_key );
  }

  void render_units( rr::Renderer& renderer,
                     Rect          covered ) const {
    render_units_impl( renderer, covered );
    // Do any post rendering steps that must be done after units
    // rendering.
    render_input_overrun_indicator( renderer, covered );
  }

  void render_non_entities( rr::Renderer& renderer ) const {
    // If the map is zoomed out enough such that some of the
    // outter space is visible, paint a background so that it
    // won't just have empty black surroundings.
    if( viewport().are_surroundings_visible() ) {
      SCOPED_RENDERER_MOD_SET(
          buffer_mods.buffer,
          rr::e_render_target_buffer::backdrop );
      render_backdrop( renderer );

      {
        // This is the shadow behind the land rectangle.
        SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, 0.5 );
        double const zoom          = viewport().get_zoom();
        int          shadow_offset = 6;
        gfx::dpoint  corner =
            viewport().landscape_buffer_render_upper_left();
        corner.x += shadow_offset;
        corner.y += shadow_offset;
        Rect const shadow_rect{
            .x = int( corner.x ),
            .y = int( corner.y ),
            .w = int( viewport().world_size_pixels().w * zoom ),
            .h = int( viewport().world_size_pixels().h * zoom ),
        };
        rr::Painter painter = renderer.painter();
        painter.draw_solid_rect(
            shadow_rect, gfx::pixel::black().with_alpha( 100 ) );
      }

      renderer.render_buffer(
          rr::e_render_target_buffer::backdrop );
    }

    // Now the actual land.
    double const      zoom = viewport().get_zoom();
    gfx::dpoint const translation =
        viewport().landscape_buffer_render_upper_left();
    renderer.set_camera( translation.distance_from_origin(),
                         zoom );
    // Should do this after setting the camera.
    renderer.render_buffer(
        rr::e_render_target_buffer::landscape );
    renderer.render_buffer(
        rr::e_render_target_buffer::landscape_annex );
  }

  void render_land_view( rr::Renderer& renderer ) const {
    render_non_entities( renderer );
    if( landview_mode_.holds<LandViewMode::hidden_terrain>() )
      return;

    // Move the rendering start slightly off screen (in the
    // upper-left direction) by an amount that is within the span
    // of one tile to partially show that tile row/column.
    gfx::dpoint const corner =
        viewport().rendering_dest_rect().origin -
        viewport().covered_pixels().origin.fmod( 32.0 ) *
            viewport().get_zoom();

    // The below render_* functions will always render at normal
    // scale and starting at 0,0 on the screen, and then the ren-
    // derer mods that we've install above will automatically do
    // the shifting and scaling.
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale,
                             viewport().get_zoom() );
    SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                             corner.distance_from_origin() );
    Rect const covered_tiles = viewport().covered_tiles();
    render_units_under_colonies( renderer, covered_tiles );
    render_native_dwellings( renderer, covered_tiles );
    render_colonies( renderer, covered_tiles );
    render_units( renderer, covered_tiles );
  }

  void draw( rr::Renderer& renderer ) const override {
    render_land_view( renderer );
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
      default: break;
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
      case input::e_input_event::quit_event: //
        break;
      case input::e_input_event::win_event: //
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

  wait<> ensure_visible( Coord const& coord ) {
    return viewport().ensure_tile_visible_smooth( coord );
  }

  wait<> center_on_tile( Coord coord ) {
    co_await viewport().center_on_tile_smooth( coord );
  }

  wait<> ensure_visible_unit( GenericUnitId id ) {
    // Need multi-ownership variant because sometimes the unit in
    // question is a worker in a colony, as can happen if we are
    // attacking an undefended colony.
    UNWRAP_CHECK( coord,
                  coord_for_unit_multi_ownership( ss_, id ) );
    co_await ensure_visible( coord );
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
    if( unit_animations_.contains( id ) ) return true;
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
    vector<GenericUnitId> sorted = unit_stack( *tile );
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
      co_await ensure_visible_unit( id );
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
      unit_animations_[id].emplace<UnitAnimation::front>();
      SCOPE_EXIT( unit_animations_.erase( id ) );
      co_await eat_cross_unit_buffered_input_events( id );
    }

    if( !last_unit_input_.has_value() ||
        last_unit_input_->unit_id != id )
      last_unit_input_ = LastUnitInput{
          .unit_id = id, .need_input_buffer_shield = true };

    // Run the blinker while waiting for user input. The question
    // is, do we want the blinking to start "on" or "off"? The
    // idea is that we want to start it off in the opposite vi-
    // sual state that the unit currently is in.
    //
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
    // we that we should start with the unit invisible.
    bool const visible_initially =
        !is_unit_visible_on_map( id ) && !eat_buffered;
    lg.trace( "visible={}, eat={}, init={}",
              is_unit_visible_on_map( id ), eat_buffered,
              visible_initially );
    LandViewPlayerInput_t input = co_await co::background(
        next_player_input_object(),
        animate_blink( id, visible_initially ) );

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

  wait<> animate_move( UnitId id, e_direction direction ) {
    // Ensure that both src and dst squares are visible.
    Coord src = coord_for_unit_indirect_or_die( ss_.units, id );
    Coord dst = src.moved( direction );
    co_await ensure_visible( src );
    // The destination square may not exist if it is a ship
    // sailing the high seas by moving off of the map edge (which
    // the original game allows).
    if( ss_.terrain.square_exists( dst ) )
      co_await ensure_visible( dst );
    play_sound_effect( e_sfx::move );
    co_await animate_slide( id, direction );
  }

  wait<> animate_attack(
      GenericUnitId attacker, GenericUnitId defender,
      vector<UnitWithDepixelateTarget_t> const& animations,
      bool                                      attacker_wins ) {
    co_await ensure_visible_unit( defender );
    co_await ensure_visible_unit( attacker );

    UNWRAP_CHECK( attacker_coord,
                  ss_.units.maybe_coord_for( attacker ) );
    UNWRAP_CHECK( defender_coord, coord_for_unit_multi_ownership(
                                      ss_, defender ) );
    UNWRAP_CHECK(
        d, attacker_coord.direction_to( defender_coord ) );

    // While the attacker is sliding we want to make sure the de-
    // fender comes to the front in case there are multiple units
    // and/or a colony on the tile.
    {
      unit_animations_[defender].emplace<UnitAnimation::front>();
      SCOPE_EXIT( unit_animations_.erase( defender ) );

      play_sound_effect( e_sfx::move );
      co_await animate_slide( attacker, d );
      // Defender unit `front` animation stops.
    }

    play_sound_effect( attacker_wins ? e_sfx::attacker_won
                                     : e_sfx::attacker_lost );
    vector<wait<>> waits;
    for( UnitWithDepixelateTarget_t const& anim : animations ) {
      GenericUnitId const id = rn::visit( anim, []( auto& o ) {
        return GenericUnitId{ o.id };
      } );
      maybe<e_tile> const target_tile =
          rn::visit( anim, []( auto& o ) {
            return o.target.fmap( []( auto type ) {
              return unit_attr( type ).tile;
            } );
          } );
      // This starts the animation.
      waits.push_back( animate_depixelation( id, target_tile ) );
    }
    // Depixelation animations have now already started. If a
    // given unit (attacker or defender) does not already have a
    // depixelation animation started then we will give it a
    // `front` animation to ensure that it remains visible for
    // the duration of the animation.
    if( !unit_animations_.contains( attacker ) )
      unit_animations_[attacker].emplace<UnitAnimation::front>();
    if( !unit_animations_.contains( defender ) )
      unit_animations_[defender].emplace<UnitAnimation::front>();
    // These will do nothing if the animations have already been
    // erased, which they will have if they were depixelation an-
    // imations.
    SCOPE_EXIT( unit_animations_.erase( attacker ) );
    SCOPE_EXIT( unit_animations_.erase( defender ) );
    co_await co::all( std::move( waits ) );
  }

  wait<> animate_colony_depixelation( Colony const& colony ) {
    co_await ensure_visible( colony.location );
    auto new_state = LandViewMode::colony_disappearing{
        .colony_id = colony.id };
    SCOPED_SET_AND_RESTORE( landview_mode_, new_state );
    // TODO: Sound effect?
    co_await animate_colony_depixelation_impl( colony );
  }

  wait<> animate_unit_depixelation(
      UnitWithDepixelateTarget_t const& what ) {
    GenericUnitId const id = rn::visit(
        what, []( auto& o ) { return GenericUnitId{ o.id }; } );
    co_await ensure_visible_unit( id );
    maybe<e_tile> const target_tile =
        rn::visit( what, []( auto& o ) {
          return o.target.fmap( []( auto type ) {
            return unit_attr( type ).tile;
          } );
        } );
    co_await animate_depixelation( id, target_tile );
  }

  // FIXME: Would be nice to make this animation a bit more so-
  // phisticated.
  wait<> animate_colony_capture(
      UnitId attacker_id, UnitId defender_id,
      vector<UnitWithDepixelateTarget_t> const& animations,
      ColonyId                                  colony_id ) {
    co_await animate_attack( attacker_id, defender_id,
                             animations,
                             /*attacker_wins=*/true );
    UNWRAP_CHECK(
        direction,
        ss_.units.coord_for( attacker_id )
            .direction_to( ss_.colonies.colony_for( colony_id )
                               .location ) );
    co_await animate_move( attacker_id, direction );
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
  return impl_->ensure_visible( coord );
}

wait<> LandViewPlane::center_on_tile( Coord coord ) {
  return impl_->center_on_tile( coord );
}

void LandViewPlane::set_visibility( maybe<e_nation> nation ) {
  return impl_->set_visibility( nation );
}

wait<> LandViewPlane::ensure_visible_unit( GenericUnitId id ) {
  return impl_->ensure_visible_unit( id );
}

wait<LandViewPlayerInput_t> LandViewPlane::get_next_input(
    UnitId id ) {
  return impl_->get_next_input( id );
}

wait<LandViewPlayerInput_t> LandViewPlane::eot_get_next_input() {
  return impl_->eot_get_next_input();
}

wait<> LandViewPlane::animate_move( UnitId      id,
                                    e_direction direction ) {
  return impl_->animate_move( id, direction );
}

wait<> LandViewPlane::animate_colony_depixelation(
    Colony const& colony ) {
  return impl_->animate_colony_depixelation( colony );
}

wait<> LandViewPlane::animate_unit_depixelation(
    UnitWithDepixelateTarget_t const& what ) {
  return impl_->animate_unit_depixelation( what );
}

wait<> LandViewPlane::animate_attack(
    GenericUnitId attacker, GenericUnitId defender,
    vector<UnitWithDepixelateTarget_t> const& animations,
    bool                                      attacker_wins ) {
  return impl_->animate_attack( attacker, defender, animations,
                                attacker_wins );
}

wait<> LandViewPlane::animate_colony_capture(
    UnitId attacker_id, UnitId defender_id,
    vector<UnitWithDepixelateTarget_t> const& animations,
    ColonyId                                  colony_id ) {
  return impl_->animate_colony_capture( attacker_id, defender_id,
                                        animations, colony_id );
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

} // namespace rn
