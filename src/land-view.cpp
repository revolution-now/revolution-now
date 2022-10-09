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
#include "ustate.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "visibility.hpp"
#include "window.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/rn.rds.hpp"
#include "config/unit-type.rds.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"

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
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation,
        gfx::to_double( gfx::size( shift ) ) );
    rr::Painter painter = renderer.painter();
    for( Rect rect : tiled_rect.to_grid_noalign( tile_size ) )
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
  unordered_map<UnitId, UnitAnimation_t>   unit_animations_;
  unordered_map<ColonyId, UnitAnimation_t> colony_animations_;
  LandViewMode_t landview_mode_ = LandViewMode::none{};
  maybe<UnitId>  last_unit_input_;

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

  wait<> animate_depixelation( UnitId            id,
                               e_depixelate_anim dp_anim ) {
    CHECK( !unit_animations_.contains( id ) );
    UnitAnimation::depixelate_unit& depixelate =
        unit_animations_[id]
            .emplace<UnitAnimation::depixelate_unit>();
    SCOPE_EXIT( {
      UNWRAP_CHECK( it, base::find( unit_animations_, id ) );
      unit_animations_.erase( it );
    } );
    depixelate.type   = dp_anim;
    depixelate.stage  = 0.0;
    depixelate.target = ss_.units.unit_for( id ).demoted_type();

    AnimThrottler throttle( kAlmostStandardFrame );
    while( depixelate.stage <= 1.0 ) {
      co_await throttle();
      depixelate.stage += config_rn.depixelate_per_frame;
    }
  }

  wait<> animate_colony_depixelation( Colony const& colony ) {
    CHECK( !colony_animations_.contains( colony.id ) );
    UnitAnimation::depixelate_colony& depixelate =
        colony_animations_[colony.id]
            .emplace<UnitAnimation::depixelate_colony>();
    SCOPE_EXIT( {
      UNWRAP_CHECK(
          it, base::find( colony_animations_, colony.id ) );
      colony_animations_.erase( it );
    } );
    depixelate.stage = 0.0;

    AnimThrottler throttle( kAlmostStandardFrame );
    while( depixelate.stage <= 1.0 ) {
      co_await throttle();
      depixelate.stage += config_rn.depixelate_per_frame;
    }
  }

  wait<> animate_blink( UnitId id ) {
    using namespace std::literals::chrono_literals;
    CHECK( !unit_animations_.contains( id ) );
    UnitAnimation::blink& blink =
        unit_animations_[id].emplace<UnitAnimation::blink>();
    // FIXME: this needs to be initially `true` for those units
    // which are not visible by default, such as e.g. a ship in a
    // colony that is asking for orders. The idea is that we want
    // this to be the opposite of the unit's default visibility
    // so that when the unit asks for orders, the player sees a
    // visual signal immediately.
    //
    // FIXME: this causes the unit to remain invisible when the
    // player continually tries to move a unit into a forbidden
    // square.
    blink = UnitAnimation::blink{ .visible = false };
    // The unit will always end up visible after we stop since we
    // are going to delete the animation object for this unit,
    // which leaves them visible by default. Do this via RAII be-
    // cause this coroutine will usually be interrupted.
    SCOPE_EXIT( {
      UNWRAP_CHECK( it, base::find( unit_animations_, id ) );
      unit_animations_.erase( it );
    } );

    AnimThrottler throttle( 500ms, /*initial_delay=*/true );
    while( true ) {
      co_await throttle();
      blink.visible = !blink.visible;
    }
  }

  wait<> animate_slide( UnitId id, e_direction d ) {
    CHECK( !unit_animations_.contains( id ) );
    Coord target =
        coord_for_unit_indirect_or_die( ss_.units, id );
    UnitAnimation::slide& mv =
        unit_animations_[id].emplace<UnitAnimation::slide>();
    SCOPE_EXIT( {
      UNWRAP_CHECK( it, base::find( unit_animations_, id ) );
      unit_animations_.erase( it );
    } );

    // TODO: make this a game option.
    double const kMaxVelocity =
        ss_.settings.fast_piece_slide ? .1 : .07;

    mv = UnitAnimation::slide{
        .target      = target.moved( d ),
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

  wait<> center_on_blinking_unit_if_any() {
    using u_i = LandViewMode::unit_input;
    auto blinking_unit =
        landview_mode_.get_if<u_i>().member( &u_i::unit_id );
    if( !blinking_unit ) {
      lg.warn(
          "There are no units currently asking for orders." );
      return make_wait<>();
    }
    return landview_ensure_visible_unit( *blinking_unit );
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
        units_from_coord_recursive( ss_.units, coord );
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
            ss_.units, planes_.window(), units, allow_activate );
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
  // Fetches one raw input and translates it, adding a new
  // element into the "translated" stream. For each translated
  // event cre- ated, preserve the time that the corresponding
  // raw input event was received.
  wait<> raw_input_translator() {
    while( !translated_input_stream_.ready() ) {
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
          translated_input_stream_.send(
              PlayerInput( LandViewPlayerInput::next_turn{},
                           raw_input.when ) );
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
          translated_input_stream_.send( PlayerInput(
              LandViewPlayerInput::european_status{},
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
            RawInput raw_input =
                co_await raw_input_stream_.next();
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
        case e::center:
          // For this one, we just perform the action right here.
          co_await center_on_blinking_unit_if_any();
          break;
      }
    }
  }

  wait<LandViewPlayerInput_t> next_player_input_object() {
    while( true ) {
      if( !translated_input_stream_.ready() )
        co_await raw_input_translator();
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

  // Given a tile, compute the screen rect where it should be
  // rendered.
  Rect render_rect_for_tile( Rect covered, Coord tile ) const {
    Delta delta_in_tiles  = tile - covered.upper_left();
    Delta delta_in_pixels = delta_in_tiles * g_tile_delta;
    return Rect::from( Coord{} + delta_in_pixels, g_tile_delta );
  }

  using UnitSkipFunc = base::function_ref<bool( UnitId )>;

  void render_units_on_square( rr::Renderer& renderer,
                               Rect covered, Coord tile,
                               UnitSkipFunc skip ) const {
    if( !viz_.visible( tile ) ) return;
    // TODO: When there are multiple units on a square, just
    // render one (which one?) and then render multiple flags
    // (stacked) to indicate that visually.
    Coord loc =
        render_rect_for_tile( covered, tile ).upper_left();
    for( UnitId id : ss_.units.from_coord( tile ) ) {
      if( skip( id ) ) continue;
      render_unit( renderer, loc, ss_.units.unit_for( id ),
                   UnitRenderOptions{ .flag   = true,
                                      .shadow = UnitShadow{} } );
    }
  }

  vector<pair<Coord, UnitId>> units_to_render(
      Rect covered ) const {
    // This is for efficiency. When we are sufficiently zoomed
    // out then it is more efficient to iterate over units then
    // covered tiles, whereas the reverse is true when zoomed in.
    unordered_map<UnitId, UnitState> const& all =
        ss_.units.all();
    int const                   num_units = all.size();
    int const                   num_tiles = covered.area();
    vector<pair<Coord, UnitId>> res;
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
      for( Coord tile : covered )
        for( UnitId id : ss_.units.from_coord( tile ) )
          res.emplace_back( tile, id );
    }
    return res;
  }

  void render_units_on_square( rr::Renderer& renderer,
                               Rect covered, Coord tile ) const {
    render_units_on_square( renderer, covered, tile,
                            []( UnitId ) { return false; } );
  }

  void render_units_default( rr::Renderer& renderer,
                             Rect          covered ) const {
    for( auto [tile, id] : units_to_render( covered ) ) {
      if( ss_.colonies.maybe_from_coord( tile ).has_value() )
        continue;
      render_units_on_square( renderer, covered, tile );
    }
  }

  void render_units_blink( rr::Renderer& renderer, Rect covered,
                           UnitId id, bool visible ) const {
    UnitId blinker_id = id;
    Coord  blink_coord =
        coord_for_unit_indirect_or_die( ss_.units, blinker_id );
    for( auto [coord, unit_id] : units_to_render( covered ) ) {
      if( coord == blink_coord ) continue;
      if( ss_.colonies.maybe_from_coord( coord ).has_value() )
        continue;
      render_units_on_square( renderer, covered, coord );
    }
    // Now render the blinking unit.
    Coord loc = render_rect_for_tile( covered, blink_coord )
                    .upper_left();
    if( visible )
      render_unit( renderer, loc,
                   ss_.units.unit_for( blinker_id ),
                   UnitRenderOptions{ .flag   = true,
                                      .shadow = UnitShadow{} } );
  }

  void render_units_during_slide(
      rr::Renderer& renderer, Rect covered, UnitId id,
      maybe<UnitId>               target_unit,
      UnitAnimation::slide const& slide ) const {
    UnitId mover_id = id;
    Coord  mover_coord =
        coord_for_unit_indirect_or_die( ss_.units, mover_id );
    maybe<Coord> target_unit_coord =
        target_unit.bind( [this]( UnitId id ) {
          return coord_for_unit_multi_ownership( ss_, id );
        } );
    // First render all units other than the sliding unit and
    // other than units on colony squares.
    for( auto const& p : units_to_render( covered ) ) {
      Coord const& coord = p.first;
      bool         has_colony =
          ss_.colonies.maybe_from_coord( coord ).has_value();
      if( has_colony && coord != target_unit_coord ) continue;
      auto skip = [&]( UnitId id ) {
        // Always draw the target unit, if any.
        if( id == target_unit ) return false;
        // On the square containing the unit being attacked, only
        // draw the unit being attacked (if any).
        if( coord == target_unit_coord ) return true;
        if( has_colony ) return true;
        if( id == mover_id ) return true;
        return false;
      };
      render_units_on_square( renderer, covered, coord, skip );
    }
    // Now render the sliding unit.
    Delta tile_delta = slide.target - mover_coord;
    CHECK( -1 <= tile_delta.w && tile_delta.w <= 1 );
    CHECK( -1 <= tile_delta.h && tile_delta.h <= 1 );
    tile_delta = tile_delta * g_tile_delta;
    Delta pixel_delta =
        tile_delta.multiply_and_round( slide.percent );
    Coord pixel_coord =
        render_rect_for_tile( covered, mover_coord )
            .upper_left();
    pixel_coord += pixel_delta;
    render_unit( renderer, pixel_coord,
                 ss_.units.unit_for( mover_id ),
                 UnitRenderOptions{ .flag   = true,
                                    .shadow = UnitShadow{} } );
  }

  void render_units_during_depixelate(
      rr::Renderer& renderer, Rect covered, UnitId depixelate_id,
      UnitAnimation::depixelate_unit const& dp_anim ) const {
    // Need the multi_ownership version because we could be de-
    // pixelating a colonist that is owned by a colony, which
    // happens when the colony is captured.
    Coord depixelate_tile =
        coord_for_unit_multi_ownership_or_die( ss_,
                                               depixelate_id );
    // First render all units other than units on colony squares
    // and unites on the same tile as the depixelating unit.
    for( auto [tile, id] : units_to_render( covered ) ) {
      if( tile == depixelate_tile ) continue;
      if( ss_.colonies.maybe_from_coord( tile ).has_value() )
        continue;
      render_units_on_square( renderer, covered, tile );
    }
    // Now render the depixelating unit.
    Coord loc = render_rect_for_tile( covered, depixelate_tile )
                    .upper_left();
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.anchor,
                             loc );
    // Check if we are depixelating to another unit.
    switch( dp_anim.type ) {
      case e_depixelate_anim::death: {
        // Render and depixelate both the unit and the flag.
        render_unit_depixelate(
            renderer, loc, ss_.units.unit_for( depixelate_id ),
            dp_anim.stage,
            UnitRenderOptions{ .flag   = true,
                               .shadow = UnitShadow{} } );
        break;
      }
      case e_depixelate_anim::demote: {
        CHECK( dp_anim.target.has_value() );
        // Render and the unit and the flag but only depixelate
        // the unit to the target unit.
        render_unit_depixelate_to(
            renderer, loc, ss_.units.unit_for( depixelate_id ),
            *dp_anim.target, dp_anim.stage,
            UnitRenderOptions{ .flag   = true,
                               .shadow = UnitShadow{} } );
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
    UNWRAP_CHECK(
        animation,
        base::lookup( colony_animations_, colony.id )
            .get_if<UnitAnimation::depixelate_colony>() );
    // As usual, the anchor coord is arbitrary so long as its po-
    // sition is fixed relative to the sprite.
    Coord anchor =
        render_rect_for_tile( covered, colony.location )
            .upper_left();
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             animation.stage );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.anchor,
                             anchor );
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
    render_units_on_square( renderer, covered, location );
  }

  // Units (rendering strategy depends on land view state).
  void render_units( rr::Renderer& renderer,
                     Rect          covered ) const {
    // The land view state should be set first, then the anima-
    // tion state; though occasionally we are in a frame where
    // the land view state has changed but the animation state
    // has not been set yet.
    if( unit_animations_.empty() ) {
      render_units_default( renderer, covered );
      return;
    }

    switch( landview_mode_.to_enum() ) {
      using namespace LandViewMode;
      case e::hidden_terrain:
      case e::none: {
        // In this case the global unit animations should always
        // be empty, in which case the if statement above should
        // have caught it.
        SHOULD_NOT_BE_HERE;
      }
      case e::unit_input: {
        auto& o = landview_mode_.get<unit_input>();
        CHECK( unit_animations_.size() == 1 );
        UNWRAP_CHECK( animation, base::lookup( unit_animations_,
                                               o.unit_id ) );
        ASSIGN_CHECK_V( blink_anim, animation,
                        UnitAnimation::blink );
        render_units_blink( renderer, covered, o.unit_id,
                            blink_anim.visible );
        break;
      }
      case e::colony_disappearing: {
        // NOTE: while the colony is depixelating it will still
        // technically exist, and so none of the units that are
        // at the gate/port will be rendered by this default unit
        // rendering method, which is what we want for the anima-
        // tion, since then we can render the last unit under-
        // neath of it (which we will have already done at this
        // point, since it is done before the colony is ren-
        // dered), which will create the effect of the colony de-
        // pixelating into a unit (if there is one; there may not
        // be, if the unit starved).
        render_units_default( renderer, covered );
        break;
      }
      case e::unit_move: {
        auto& o = landview_mode_.get<unit_move>();
        CHECK( unit_animations_.size() == 1 );
        UNWRAP_CHECK( animation, base::lookup( unit_animations_,
                                               o.unit_id ) );
        ASSIGN_CHECK_V( slide_anim, animation,
                        UnitAnimation::slide );
        render_units_during_slide( renderer, covered, o.unit_id,
                                   /*target_unit=*/nothing,
                                   slide_anim );
        break;
      }
      case e::unit_attack: {
        auto& o = landview_mode_.get<unit_attack>();
        CHECK( unit_animations_.size() == 1 );
        UnitId anim_id = unit_animations_.begin()->first;
        UnitAnimation_t const& animation =
            unit_animations_.begin()->second;
        if( auto slide_anim =
                animation.get_if<UnitAnimation::slide>() ) {
          CHECK( anim_id == o.attacker );
          render_units_during_slide( renderer, covered,
                                     o.attacker, o.defender,
                                     *slide_anim );
          break;
        }
        if( auto dp_anim =
                animation
                    .get_if<UnitAnimation::depixelate_unit>() ) {
          render_units_during_depixelate( renderer, covered,
                                          anim_id, *dp_anim );
          break;
        }
        FATAL(
            "Unit animation not found for either slide or "
            "depixelate." );
      }
    }
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
        int          shadow_offset = int( 40 * zoom );
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
          lg.debug( "received key from lua: {}", lua_orders );
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

  void landview_reset_input_buffers() {
    raw_input_stream_.reset();
    translated_input_stream_.reset();
  }

  void landview_start_new_turn() {
    // An example of why this is needed is because when a unit is
    // moving (say, it is the only active unit) and the screen
    // scrolls away from it to show a colony update, then when
    // that update message closes and the next turn starts and
    // focuses again on the unit, it would not scroll back to
    // that unit.
    g_needs_scroll_to_unit_on_input = true;
  }

  wait<> landview_ensure_visible( Coord const& coord ) {
    return viewport().ensure_tile_visible_smooth( coord );
  }

  wait<> landview_ensure_visible_unit( UnitId id ) {
    // Need multi-ownership variant because sometimes the unit in
    // question is a worker in a colony, as can happen if we are
    // attacking an undefended colony.
    UNWRAP_CHECK( coord,
                  coord_for_unit_multi_ownership( ss_, id ) );
    return landview_ensure_visible( coord );
  }

  void set_visibility( maybe<e_nation> nation ) {
    viz_ = Visibility::create( ss_, nation );
  }

  void zoom_out_full() {
    viewport().set_zoom( viewport().optimal_min_zoom() );
  }

  wait<LandViewPlayerInput_t> landview_get_next_input(
      UnitId id ) {
    // When we start on a new unit clear the input queue so that
    // commands that were accidentally buffered while controlling
    // the previous unit don't affect this new one, which would
    // almost certainly not be desirable. Also, we only pan to
    // the unit here because if we did that outside of this if
    // statement then the viewport would pan to the blinking unit
    // after the player e.g. clicks on another unit to activate
    // it.
    if( last_unit_input_ != id )
      g_needs_scroll_to_unit_on_input = true;

    // This might be true either because we started a new turn,
    // or because of the above assignment.
    if( g_needs_scroll_to_unit_on_input )
      co_await landview_ensure_visible_unit( id );
    g_needs_scroll_to_unit_on_input = false;

    if( last_unit_input_ != id ) landview_reset_input_buffers();
    last_unit_input_ = id;

    SCOPED_SET_AND_RESTORE(
        landview_mode_,
        LandViewMode::unit_input{ .unit_id = id } );

    // Run the blinker while waiting for user input.
    co_return co_await co::background(
        next_player_input_object(), animate_blink( id ) );
  }

  wait<LandViewPlayerInput_t> landview_eot_get_next_input() {
    last_unit_input_ = nothing;
    landview_mode_   = LandViewMode::none{};
    return next_player_input_object();
  }

  wait<> landview_animate_move( UnitId      id,
                                e_direction direction ) {
    // Ensure that both src and dst squares are visible.
    Coord src = coord_for_unit_indirect_or_die( ss_.units, id );
    Coord dst = src.moved( direction );
    co_await landview_ensure_visible( src );
    // The destination square may not exist if it is a ship
    // sailing the high seas by moving off of the map edge (which
    // the original game allows).
    if( ss_.terrain.square_exists( dst ) )
      co_await landview_ensure_visible( dst );
    SCOPED_SET_AND_RESTORE(
        landview_mode_,
        LandViewMode::unit_move{ .unit_id = id } );
    play_sound_effect( e_sfx::move );
    co_await animate_slide( id, direction );
  }

  wait<> landview_animate_attack( UnitId attacker,
                                  UnitId defender,
                                  bool   attacker_wins,
                                  e_depixelate_anim dp_anim ) {
    co_await landview_ensure_visible_unit( defender );
    co_await landview_ensure_visible_unit( attacker );
    auto new_state = LandViewMode::unit_attack{
        .attacker      = attacker,
        .defender      = defender,
        .attacker_wins = attacker_wins };
    SCOPED_SET_AND_RESTORE( landview_mode_, new_state );
    UNWRAP_CHECK( attacker_coord,
                  ss_.units.maybe_coord_for( attacker ) );
    UNWRAP_CHECK( defender_coord, coord_for_unit_multi_ownership(
                                      ss_, defender ) );
    UNWRAP_CHECK(
        d, attacker_coord.direction_to( defender_coord ) );
    play_sound_effect( e_sfx::move );
    co_await animate_slide( attacker, d );

    play_sound_effect( attacker_wins ? e_sfx::attacker_won
                                     : e_sfx::attacker_lost );
    co_await animate_depixelation(
        attacker_wins ? defender : attacker, dp_anim );
  }

  wait<> landview_animate_colony_depixelation(
      Colony const& colony ) {
    co_await landview_ensure_visible( colony.location );
    auto new_state = LandViewMode::colony_disappearing{
        .colony_id = colony.id };
    SCOPED_SET_AND_RESTORE( landview_mode_, new_state );
    // TODO: Sound effect?
    co_await animate_colony_depixelation( colony );
  }

  // FIXME: Would be nice to make this animation a bit more so-
  // phisticated, but we first need to fix the animation frame-
  // work in this module to be more flexible.
  wait<> landview_animate_colony_capture( UnitId   attacker_id,
                                          UnitId   defender_id,
                                          ColonyId colony_id ) {
    co_await landview_animate_attack( attacker_id, defender_id,
                                      /*attacker_wins=*/true,
                                      e_depixelate_anim::death );
    UNWRAP_CHECK(
        direction,
        ss_.units.coord_for( attacker_id )
            .direction_to( ss_.colonies.colony_for( colony_id )
                               .location ) );
    co_await landview_animate_move( attacker_id, direction );
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

wait<> LandViewPlane::landview_ensure_visible(
    Coord const& coord ) {
  return impl_->landview_ensure_visible( coord );
}

void LandViewPlane::set_visibility( maybe<e_nation> nation ) {
  return impl_->set_visibility( nation );
}

wait<> LandViewPlane::landview_ensure_visible_unit( UnitId id ) {
  return impl_->landview_ensure_visible_unit( id );
}

wait<LandViewPlayerInput_t>
LandViewPlane::landview_get_next_input( UnitId id ) {
  return impl_->landview_get_next_input( id );
}

wait<LandViewPlayerInput_t>
LandViewPlane::landview_eot_get_next_input() {
  return impl_->landview_eot_get_next_input();
}

wait<> LandViewPlane::landview_animate_move(
    UnitId id, e_direction direction ) {
  return impl_->landview_animate_move( id, direction );
}

wait<> LandViewPlane::landview_animate_colony_depixelation(
    Colony const& colony ) {
  return impl_->landview_animate_colony_depixelation( colony );
}

wait<> LandViewPlane::landview_animate_attack(
    UnitId attacker, UnitId defender, bool attacker_wins,
    e_depixelate_anim dp_anim ) {
  return impl_->landview_animate_attack(
      attacker, defender, attacker_wins, dp_anim );
}

wait<> LandViewPlane::landview_animate_colony_capture(
    UnitId attacker_id, UnitId defender_id,
    ColonyId colony_id ) {
  return impl_->landview_animate_colony_capture(
      attacker_id, defender_id, colony_id );
}

void LandViewPlane::landview_reset_input_buffers() {
  return impl_->landview_reset_input_buffers();
}

void LandViewPlane::landview_start_new_turn() {
  return impl_->landview_start_new_turn();
}

void LandViewPlane::zoom_out_full() {
  return impl_->zoom_out_full();
}

} // namespace rn
