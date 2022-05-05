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
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "colony-id.hpp"
#include "compositor.hpp"
#include "config-files.hpp"
#include "coord.hpp"
#include "cstate.hpp"
#include "game-state.hpp"
#include "gs-land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "map-gen.hpp" // FIXME: temporary
#include "orders.hpp"
#include "physics.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "render.hpp"
#include "renderer.hpp" // FIXME: remove
#include "road.hpp"
#include "screen.hpp"
#include "sound.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "unit-id.hpp"
#include "ustate.hpp"
#include "utype.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "window.hpp"

// config
#include "config/land-view.rds.hpp"

// render
#include "render/renderer.hpp"

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
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// Rds
#include "land-view-impl.rds.hpp"

// Revolution Now (config)
#include "../config/rcl/rn.inl"

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

co::stream<RawInput>                   g_raw_input_stream;
co::stream<PlayerInput>                g_translated_input_stream;
unordered_map<UnitId, UnitAnimation_t> g_unit_animations;
LandViewUnitActionState_t              g_landview_state =
    LandViewUnitActionState::none{};
maybe<UnitId> g_last_unit_input;

bool g_needs_scroll_to_unit_on_input = true;

SmoothViewport& viewport() {
  return GameState::land_view().viewport;
}

/****************************************************************
** Land-View Rendering
*****************************************************************/
struct LandViewRenderer {
  // Given a tile, compute the screen rect where it should be
  // rendered.
  Rect render_rect_for_tile( Coord tile ) {
    Delta delta_in_tiles  = tile - covered.upper_left();
    Delta delta_in_pixels = delta_in_tiles * g_tile_scale;
    return Rect::from( Coord{} + delta_in_pixels, g_tile_delta );
  }

  using UnitSkipFunc = base::function_ref<bool( UnitId )>;

  void render_units_on_square( Coord tile, UnitSkipFunc skip ) {
    // TODO: When there are multiple units on a square, just
    // render one (which one?) and then render multiple flags
    // (stacked) to indicate that visually.
    Coord loc = render_rect_for_tile( tile ).upper_left();
    for( UnitId id : units_from_coord( tile ) ) {
      if( skip( id ) ) continue;
      render_unit( renderer, loc, id, /*with_icon=*/true );
    }
  }

  void render_units_on_square( Coord tile ) {
    render_units_on_square( tile,
                            []( UnitId ) { return false; } );
  }

  void render_units_default() {
    for( Coord tile : covered ) {
      if( colony_from_coord( tile ).has_value() ) continue;
      render_units_on_square( tile );
    }
  }

  void render_units_blink( UnitId id, bool visible ) {
    UnitId blinker_id = id;
    Coord  blink_coord =
        coord_for_unit_indirect_or_die( blinker_id );
    for( auto coord : covered ) {
      if( coord == blink_coord ) continue;
      if( colony_from_coord( coord ).has_value() ) continue;
      render_units_on_square( coord );
    }
    // Now render the blinking unit.
    Coord loc = render_rect_for_tile( blink_coord ).upper_left();
    if( visible )
      render_unit( renderer, loc, blinker_id,
                   /*with_icon=*/true );
  }

  void render_units_during_slide(
      UnitId id, maybe<UnitId> target_unit,
      UnitAnimation::slide const& slide ) {
    UnitId mover_id = id;
    Coord  mover_coord =
        coord_for_unit_indirect_or_die( mover_id );
    maybe<Coord> target_unit_coord =
        target_unit.bind( coord_for_unit_multi_ownership );
    // First render all units other than the sliding unit and
    // other than units on colony squares.
    for( auto coord : covered ) {
      bool has_colony = colony_from_coord( coord ).has_value();
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
      render_units_on_square( coord, skip );
    }
    // Now render the sliding unit.
    Delta tile_delta = slide.target - mover_coord;
    CHECK( -1 <= tile_delta.w && tile_delta.w <= 1 );
    CHECK( -1 <= tile_delta.h && tile_delta.h <= 1 );
    tile_delta *= g_tile_scale;
    Delta pixel_delta =
        tile_delta.multiply_and_round( slide.percent );
    Coord pixel_coord =
        render_rect_for_tile( mover_coord ).upper_left();
    pixel_coord += pixel_delta;
    render_unit( renderer, pixel_coord, mover_id,
                 /*with_icon=*/true );
  }

  void render_units_during_depixelate(
      UnitId                           depixelate_id,
      UnitAnimation::depixelate const& dp_anim ) {
    // Need the multi_ownership version because we could be de-
    // pixelating a colonist that is owned by a colony, which
    // happens when the colony is captured.
    Coord depixelate_tile =
        coord_for_unit_multi_ownership_or_die( depixelate_id );
    // First render all units other than units on colony squares
    // and unites on the same tile as the depixelating unit.
    for( Coord tile : covered ) {
      if( tile == depixelate_tile ) continue;
      if( colony_from_coord( tile ).has_value() ) continue;
      render_units_on_square( tile );
    }
    // Now render the depixelating unit.
    Coord loc =
        render_rect_for_tile( depixelate_tile ).upper_left();
    SCOPED_RENDERER_MOD( painter_mods.depixelate.anchor, loc );
    rr::Painter painter = renderer.painter();
    // Check if we are depixelating to another unit.
    switch( dp_anim.type ) {
      case e_depixelate_anim::death: {
        // Render and depixelate both the unit and the flag.
        SCOPED_RENDERER_MOD( painter_mods.depixelate.stage,
                             dp_anim.stage );
        render_unit( renderer, loc, depixelate_id,
                     /*with_icon=*/true );
        break;
      }
      case e_depixelate_anim::demote: {
        CHECK( dp_anim.target.has_value() );
        e_tile from_tile =
            unit_from_id( depixelate_id ).desc().tile;
        e_tile    to_tile = unit_attr( *dp_anim.target ).tile;
        gfx::size target =
            depixelation_offset( painter, from_tile, to_tile );
        // Render the flag first so that we don't subject it to
        // the depixelation.
        render_nationality_icon( renderer, loc, depixelate_id );
        SCOPED_RENDERER_MOD( painter_mods.depixelate.stage,
                             dp_anim.stage );
        SCOPED_RENDERER_MOD( painter_mods.depixelate.target,
                             target );
        render_unit( renderer, loc, depixelate_id,
                     /*with_icon=*/false );
        break;
      }
    }
  }

  void render_colonies() {
    rr::Painter painter = renderer.painter();
    // FIXME: since colony icons spill over the usual 32x32 tile
    // we need to render colonies that are beyond the `covered`
    // rect.
    for( Coord tile : covered ) {
      maybe<ColonyId> col_id = colony_from_coord( tile );
      if( !col_id.has_value() ) continue;
      Colony const& colony = colony_from_id( *col_id );

      Coord tile_coord =
          render_rect_for_tile( tile ).upper_left();
      Coord colony_sprite_upper_left =
          tile_coord - Delta{ 6_w, 6_h };
      render_colony( painter, colony_sprite_upper_left,
                     *col_id );
      Coord name_coord =
          tile_coord + config_land_view.colony_name_offset;
      render_text_markup(
          renderer, name_coord,
          config_land_view.colony_name_font,
          TextMarkupInfo{
              .shadowed_text_color   = gfx::pixel::white(),
              .shadowed_shadow_color = gfx::pixel::black() },
          fmt::format( "@[S]{}@[]", colony.name() ) );
    }
  }

  // Units (rendering strategy depends on land view state).
  void render_units() {
    // The land view state should be set first, then the anima-
    // tion state; though occasionally we are in a frame where
    // the land view state has changed but the animation state
    // has not been set yet.
    if( g_unit_animations.empty() ) {
      render_units_default();
      return;
    }

    switch( g_landview_state.to_enum() ) {
      using namespace LandViewUnitActionState;
      case e::none: {
        // In this case the global unit animations should always
        // be empty, in which case the if statement above should
        // have caught it.
        SHOULD_NOT_BE_HERE;
      }
      case e::unit_input: {
        auto& o = g_landview_state.get<unit_input>();
        CHECK( g_unit_animations.size() == 1 );
        UNWRAP_CHECK( animation, base::lookup( g_unit_animations,
                                               o.unit_id ) );
        ASSIGN_CHECK_V( blink_anim, animation,
                        UnitAnimation::blink );
        render_units_blink( o.unit_id, blink_anim.visible );
        break;
      }
      case e::unit_move: {
        auto& o = g_landview_state.get<unit_move>();
        CHECK( g_unit_animations.size() == 1 );
        UNWRAP_CHECK( animation, base::lookup( g_unit_animations,
                                               o.unit_id ) );
        ASSIGN_CHECK_V( slide_anim, animation,
                        UnitAnimation::slide );
        render_units_during_slide( o.unit_id,
                                   /*target_unit=*/nothing,
                                   slide_anim );
        break;
      }
      case e::unit_attack: {
        auto& o = g_landview_state.get<unit_attack>();
        CHECK( g_unit_animations.size() == 1 );
        UnitId anim_id = g_unit_animations.begin()->first;
        UnitAnimation_t const& animation =
            g_unit_animations.begin()->second;
        if( auto slide_anim =
                animation.get_if<UnitAnimation::slide>() ) {
          CHECK( anim_id == o.attacker );
          render_units_during_slide( o.attacker, o.defender,
                                     *slide_anim );
          break;
        }
        if( auto dp_anim =
                animation.get_if<UnitAnimation::depixelate>() ) {
          render_units_during_depixelate( anim_id, *dp_anim );
          break;
        }
        FATAL(
            "Unit animation not found for either slide or "
            "depixelate." );
        break;
      }
    }
  }

  rr::Renderer& renderer;
  Rect const    covered = {};
};

void render_land_view( rr::Renderer& renderer ) {
  double zoom = viewport().get_zoom();
  renderer.set_camera( viewport()
                           .landscape_buffer_render_upper_left()
                           .distance_from_origin(),
                       zoom );
  // Should do this after setting the camera.
  renderer.render_buffer(
      rr::e_render_target_buffer::landscape );

  Coord corner = viewport().rendering_dest_rect().upper_left();
  Delta hidden =
      viewport().covered_pixels().upper_left() % g_tile_scale;
  if( hidden != Delta{} ) {
    DCHECK( hidden.w >= 0_w );
    DCHECK( hidden.h >= 0_h );
    // Move the rendering start slightly off screen (in the
    // upper-left direction) by an amount that is within the span
    // of one tile to partially show that tile row/column.
    corner -= hidden.multiply_and_round( zoom );
  }

  LandViewRenderer lv_renderer{
      .renderer = renderer,
      .covered  = viewport().covered_tiles(),
  };

  // The below render_* functions will always render at normal
  // scale and starting at 0,0 on the screen, and then the ren-
  // derer mods that we've install above will automatically do
  // the shifting and scaling.
  SCOPED_RENDERER_MOD( painter_mods.repos.scale, zoom );
  SCOPED_RENDERER_MOD( painter_mods.repos.translation,
                       corner.distance_from_origin() );
  lv_renderer.render_colonies();
  lv_renderer.render_units();
}

/****************************************************************
** Animations
*****************************************************************/
wait<> animate_depixelation( UnitId            id,
                             e_depixelate_anim dp_anim ) {
  CHECK( !g_unit_animations.contains( id ) );
  UnitAnimation::depixelate& depixelate =
      g_unit_animations[id].emplace<UnitAnimation::depixelate>();
  SCOPE_EXIT( {
    UNWRAP_CHECK( it, base::find( g_unit_animations, id ) );
    g_unit_animations.erase( it );
  } );
  depixelate.type   = dp_anim;
  depixelate.stage  = 0.0;
  depixelate.target = unit_from_id( id ).demoted_type();

  AnimThrottler throttle( kAlmostStandardFrame );
  while( depixelate.stage <= 1.0 ) {
    co_await throttle();
    depixelate.stage += config_rn.depixelate_per_frame;
  }
}

wait<> animate_blink( UnitId id ) {
  using namespace std::literals::chrono_literals;
  CHECK( !g_unit_animations.contains( id ) );
  UnitAnimation::blink& blink =
      g_unit_animations[id].emplace<UnitAnimation::blink>();
  // FIXME: this needs to be initially `true` for those units
  // which are not visible by default, such as e.g. a ship in a
  // colony that is asking for orders. The idea is that we want
  // this to be the opposite of the unit's default visibility so
  // that when the unit asks for orders, the player sees a visual
  // signal immediately.
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
    UNWRAP_CHECK( it, base::find( g_unit_animations, id ) );
    g_unit_animations.erase( it );
  } );

  AnimThrottler throttle( 500ms, /*initial_delay=*/true );
  while( true ) {
    co_await throttle();
    blink.visible = !blink.visible;
  }
}

wait<> animate_slide( UnitId id, e_direction d ) {
  CHECK( !g_unit_animations.contains( id ) );
  Coord target = coord_for_unit_indirect_or_die( id );
  UnitAnimation::slide& mv =
      g_unit_animations[id].emplace<UnitAnimation::slide>();
  SCOPE_EXIT( {
    UNWRAP_CHECK( it, base::find( g_unit_animations, id ) );
    g_unit_animations.erase( it );
  } );
  mv = UnitAnimation::slide{
      // FIXME: check if target is in world.
      .target      = target.moved( d ),
      .percent     = 0.0,
      .percent_vel = DissipativeVelocity{
          /*min_velocity=*/0,            //
          /*max_velocity=*/.07,          //
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
  using u_i = LandViewUnitActionState::unit_input;
  auto blinking_unit =
      g_landview_state.get_if<u_i>().member( &u_i::unit_id );
  if( !blinking_unit ) {
    lg.warn( "There are no units currently asking for orders." );
    return make_wait<>();
  }
  return landview_ensure_visible( *blinking_unit );
}

/****************************************************************
** Tile Clicking
*****************************************************************/
// If there is a single unit on the square with orders and
// allow_activate is false then the unit's orders will be
// cleared.
//
// If there is a single unit on the square with orders and
// allow_activate is true then the unit's orders will be cleared
// and the unit will be placed at the back of the queue to poten-
// tially move this turn.
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
// them, with the results for each unit behaving in a similar way
// to the single-unit case described above with respect to orders
// and the allow_activate flag.
wait<vector<LandViewPlayerInput_t>> click_on_world_tile(
    Coord coord ) {
  vector<LandViewPlayerInput_t> res;
  auto add = [&res]<typename T>( T t ) -> T& {
    res.push_back( std::move( t ) );
    return res.back().get<T>();
  };

  bool allow_activate =
      g_landview_state
          .holds<LandViewUnitActionState::unit_input>();

  // First check for colonies.
  if( auto maybe_id = colony_from_coord( coord ); maybe_id ) {
    auto& colony = add( LandViewPlayerInput::colony{} );
    colony.id    = *maybe_id;
    co_return res;
  }

  // Now check for units.
  auto const& units = units_from_coord_recursive( coord );
  if( units.size() == 0 ) co_return res;

  // Decide which units are selected and for what actions.
  vector<ui::UnitSelection> selections;
  if( units.size() == 1 ) {
    auto              id = *units.begin();
    ui::UnitSelection selection{
        id, ui::e_unit_selection::clear_orders };
    if( !unit_from_id( id ).has_orders() && allow_activate )
      selection.what = ui::e_unit_selection::activate;
    selections = vector{ selection };
  } else {
    selections =
        co_await ui::unit_selection_box( units, allow_activate );
  }

  vector<UnitId> prioritize;
  for( auto const& selection : selections ) {
    switch( selection.what ) {
      case ui::e_unit_selection::clear_orders:
        unit_from_id( selection.id ).clear_orders();
        break;
      case ui::e_unit_selection::activate:
        CHECK( allow_activate );
        // Activation implies also to clear orders if they're not
        // already cleared. We do this here because, even if the
        // prioritization is later denied (because the unit has
        // already moved this turn) the clearing of the orders
        // should still be upheld, because that can always be
        // done, hence they are done separately.
        unit_from_id( selection.id ).clear_orders();
        prioritize.push_back( selection.id );
        break;
    }
  }
  // These need to all be grouped into a vector so that the first
  // prioritized unit doesn't start asking for orders before the
  // rest are prioritized.
  if( !prioritize.empty() )
    add( LandViewPlayerInput::prioritize{} ).units =
        std::move( prioritize );

  co_return res;
}

/****************************************************************
** Input Processor
*****************************************************************/
// Fetches one raw input and translates it, adding a new element
// into the "translated" stream. For each translated event cre-
// ated, preserve the time that the corresponding raw input event
// was received.
wait<> raw_input_translator() {
  while( !g_translated_input_stream.ready() ) {
    RawInput raw_input = co_await g_raw_input_stream.next();

    switch( raw_input.input.to_enum() ) {
      using namespace LandViewRawInput;
      case e::orders: {
        g_translated_input_stream.send( PlayerInput(
            LandViewPlayerInput::give_orders{
                .orders = raw_input.input
                              .get<LandViewRawInput::orders>()
                              .orders },
            raw_input.when ) );
        break;
      }
      case e::tile_click: {
        auto& o = raw_input.input.get<tile_click>();
        vector<LandViewPlayerInput_t> inputs =
            co_await click_on_world_tile( o.coord );
        // Since we may have just popped open a box to ask the
        // user to select units, just use the "now" time so that
        // these events don't get disgarded. Also, mouse clicks
        // are not likely to get buffered for too long anyway.
        for( auto const& input : inputs )
          g_translated_input_stream.send(
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
    if( !g_translated_input_stream.ready() )
      co_await raw_input_translator();
    PlayerInput res = co_await g_translated_input_stream.next();
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

  viewport().fix_zoom_rounding();

  // TODO: should only do the following when the viewport has
  // input focus.
  auto const* __state = ::SDL_GetKeyboardState( nullptr );

  // Returns true if key is pressed.
  auto state = [__state]( ::SDL_Scancode code ) {
    return __state[code] != 0;
  };

  if( state( ::SDL_SCANCODE_LSHIFT ) ) {
    viewport().set_x_push(
        state( ::SDL_SCANCODE_A )   ? e_push_direction::negative
        : state( ::SDL_SCANCODE_D ) ? e_push_direction::positive
                                    : e_push_direction::none );
    // y motion
    viewport().set_y_push(
        state( ::SDL_SCANCODE_W )   ? e_push_direction::negative
        : state( ::SDL_SCANCODE_S ) ? e_push_direction::positive
                                    : e_push_direction::none );

    if( state( ::SDL_SCANCODE_A ) || state( ::SDL_SCANCODE_D ) ||
        state( ::SDL_SCANCODE_W ) || state( ::SDL_SCANCODE_S ) )
      viewport().stop_auto_panning();
  }
}

maybe<orders_t> try_orders_from_lua( int keycode, bool ctrl_down,
                                     bool shf_down ) {
  lua::state& st = lua_global_state();

  lua::any lua_orders = st["land_view"]["key_to_orders"](
      keycode, ctrl_down, shf_down );
  if( !lua_orders ) return nothing;

  // FIXME: the conversion from Lua to C++ below needs to be au-
  // tomated once we have sumtype and struct reflection. When
  // this is done then ideally we will just be able to do this:
  //
  //   orders_t orders = lua_orders.as<orders_t>();
  //
  // And it should do the correct conversion and error checking.
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

struct LandViewPlane : public Plane {
  LandViewPlane() = default;
  bool covers_screen() const override { return true; }
  void initialize( IMapUpdater& ) override {
    // Initialize general global data.
    g_unit_animations.clear();
    g_landview_state  = LandViewUnitActionState::none{};
    g_last_unit_input = nothing;
    g_raw_input_stream.reset();
    g_translated_input_stream.reset();
    g_needs_scroll_to_unit_on_input = true;
    // This is done to initialize the viewport with info about
    // the viewport size that cannot be known while it is being
    // constructed.
    advance_viewport_state();
  }
  void advance_state() override { advance_viewport_state(); }
  void draw( rr::Renderer& renderer ) const override {
    render_land_view( renderer );
  }
  maybe<Plane::MenuClickHandler> menu_click_handler(
      e_menu_item item ) const override {
    // These are factors by which the zoom will be scaled when
    // zooming in/out with the menus.
    double constexpr zoom_in_factor  = 2.0;
    double constexpr zoom_out_factor = 1.0 / zoom_in_factor;
    // This is so that a zoom-in followed by a zoom-out will
    // re- store to previous state.
    static_assert( zoom_in_factor * zoom_out_factor == 1.0 );
    if( item == e_menu_item::zoom_in ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        viewport().stop_auto_zoom();
        viewport().stop_auto_panning();
        viewport().smooth_zoom_target( viewport().get_zoom() *
                                       zoom_in_factor );
      };
      return handler;
    }
    if( item == e_menu_item::zoom_out ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        viewport().stop_auto_zoom();
        viewport().stop_auto_panning();
        viewport().smooth_zoom_target( viewport().get_zoom() *
                                       zoom_out_factor );
      };
      return handler;
    }
    if( item == e_menu_item::restore_zoom ) {
      if( viewport().get_zoom() == 1.0 ) return nothing;
      static Plane::MenuClickHandler handler = [] {
        viewport().smooth_zoom_target( 1.0 );
      };
      return handler;
    }
    if( item == e_menu_item::find_blinking_unit ) {
      if( !g_landview_state
               .holds<LandViewUnitActionState::unit_input>() )
        return nothing;
      static Plane::MenuClickHandler handler = [] {
        g_raw_input_stream.send(
            RawInput( LandViewRawInput::center{} ) );
      };
      return handler;
    }
    return nothing;
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
        auto& val = event.get<input::key_event_t>();
        // TODO: Need to put this in the input module.
        auto const* __state = ::SDL_GetKeyboardState( nullptr );
        auto        state   = [__state]( ::SDL_Scancode code ) {
          return __state[code] != 0;
        };
        // This is because we need to distinguish uppercase
        // from lowercase.
        if( state( ::SDL_SCANCODE_LSHIFT ) ||
            state( ::SDL_SCANCODE_RSHIFT ) )
          break;

        auto& key_event = val;
        if( key_event.change != input::e_key_change::down )
          break;
        handled = e_input_handled::yes;
        // First allow the Lua hook to handle the key press if it
        // wants.
        maybe<orders_t> lua_orders = try_orders_from_lua(
            key_event.keycode, key_event.mod.ctrl_down,
            key_event.mod.shf_down );
        if( lua_orders ) {
          lg.debug( "received key from lua: {}", lua_orders );
          g_raw_input_stream.send(
              RawInput( LandViewRawInput::orders{
                  .orders = *lua_orders } ) );
          break;
        }
        switch( key_event.keycode ) {
          case ::SDLK_z:
            if( viewport().get_zoom() < 1.0 )
              viewport().smooth_zoom_target( 1.0 );
            else if( viewport().get_zoom() < 1.5 )
              viewport().smooth_zoom_target( 2.0 );
            else
              viewport().smooth_zoom_target( 1.0 );
            break;
          case ::SDLK_w:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::wait{} } ) );
            break;
          case ::SDLK_s:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::sentry{} } ) );
            break;
          case ::SDLK_f:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::fortify{} } ) );
            break;
          case ::SDLK_b:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::build{} } ) );
            break;
          case ::SDLK_c:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::center{} ) );
            break;
          case ::SDLK_d:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::disband{} } ) );
            break;
          case ::SDLK_g: {
            MapUpdater map_updater(
                GameState::terrain(),
                global_renderer_use_only_when_needed() );
            generate_terrain( map_updater );
            break;
          }
          case ::SDLK_SPACE:
          case ::SDLK_KP_5:
            g_raw_input_stream.send(
                RawInput( LandViewRawInput::orders{
                    .orders = orders::forfeight{} } ) );
            break;
          default:
            handled = e_input_handled::no;
            if( key_event.direction ) {
              g_raw_input_stream.send(
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
        // If the mouse is in the viewport and its a wheel
        // event then we are in business.
        if( viewport().screen_coord_in_viewport( val.pos ) ) {
          if( val.wheel_delta < 0 )
            viewport().set_zoom_push( e_push_direction::negative,
                                      nothing );
          if( val.wheel_delta > 0 )
            viewport().set_zoom_push( e_push_direction::positive,
                                      val.pos );
          // A user zoom request halts any auto zooming that
          // may currently be happening.
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
        if( auto maybe_tile =
                viewport().screen_pixel_to_world_tile(
                    val.pos ) ) {
          lg.debug( "clicked on tile: {}.", *maybe_tile );
          g_raw_input_stream.send(
              RawInput( LandViewRawInput::tile_click{
                  .coord = *maybe_tile } ) );
          handled = e_input_handled::yes;
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
};

LandViewPlane g_land_view_plane;

} // namespace

Plane* land_view_plane() { return &g_land_view_plane; }

/****************************************************************
** Public API
*****************************************************************/
void landview_reset_input_buffers() {
  g_raw_input_stream.reset();
  g_translated_input_stream.reset();
}

void landview_start_new_turn() {
  // An example of why this is needed is because when a unit is
  // moving (say, it is the only active unit) and the screen
  // scrolls away from it to show a colony update, then when that
  // update message closes and the next turn starts and focuses
  // again on the unit, it would not scroll back to that unit.
  g_needs_scroll_to_unit_on_input = true;
}

wait<> landview_ensure_visible( Coord const& coord ) {
  return viewport().ensure_tile_visible_smooth( coord );
}

wait<> landview_ensure_visible( UnitId id ) {
  // Need multi-ownership variant because sometimes the unit in
  // question is a worker in a colony, as can happen if we are
  // attacking an undefended colony.
  UNWRAP_CHECK( coord, coord_for_unit_multi_ownership( id ) );
  return landview_ensure_visible( coord );
}

wait<LandViewPlayerInput_t> landview_get_next_input(
    UnitId id ) {
  // When we start on a new unit clear the input queue so that
  // commands that were accidentally buffered while controlling
  // the previous unit don't affect this new one, which would al-
  // most certainly not be desirable. Also, we only pan to the
  // unit here because if we did that outside of this if state-
  // ment then the viewport would pan to the blinking unit after
  // the player e.g. clicks on another unit to activate it.
  if( g_last_unit_input != id )
    g_needs_scroll_to_unit_on_input = true;

  // This might be true either because we started a new turn, or
  // because of the above assignment.
  if( g_needs_scroll_to_unit_on_input )
    co_await landview_ensure_visible( id );
  g_needs_scroll_to_unit_on_input = false;

  if( g_last_unit_input != id ) landview_reset_input_buffers();
  g_last_unit_input = id;

  SCOPED_SET_AND_RESTORE(
      g_landview_state,
      LandViewUnitActionState::unit_input{ .unit_id = id } );

  // Run the blinker while waiting for user input.
  co_return co_await co::background( next_player_input_object(),
                                     animate_blink( id ) );
}

wait<LandViewPlayerInput_t> landview_eot_get_next_input() {
  g_last_unit_input = nothing;
  g_landview_state  = LandViewUnitActionState::none{};
  return next_player_input_object();
}

wait<> landview_animate_move( UnitId      id,
                              e_direction direction ) {
  // Ensure that both src and dst squares are visible.
  Coord src = coord_for_unit_indirect_or_die( id );
  Coord dst = src.moved( direction );
  co_await landview_ensure_visible( src );
  co_await landview_ensure_visible( dst );
  SCOPED_SET_AND_RESTORE(
      g_landview_state,
      LandViewUnitActionState::unit_move{ .unit_id = id } );
  play_sound_effect( e_sfx::move );
  co_await animate_slide( id, direction );
}

wait<> landview_animate_attack( UnitId attacker, UnitId defender,
                                bool              attacker_wins,
                                e_depixelate_anim dp_anim ) {
  co_await landview_ensure_visible( defender );
  co_await landview_ensure_visible( attacker );
  auto new_state = LandViewUnitActionState::unit_attack{
      .attacker      = attacker,
      .defender      = defender,
      .attacker_wins = attacker_wins };
  SCOPED_SET_AND_RESTORE( g_landview_state, new_state );
  UNWRAP_CHECK( attacker_coord, coord_for_unit( attacker ) );
  UNWRAP_CHECK( defender_coord,
                coord_for_unit_multi_ownership( defender ) );
  UNWRAP_CHECK( d,
                attacker_coord.direction_to( defender_coord ) );
  play_sound_effect( e_sfx::move );
  co_await animate_slide( attacker, d );

  play_sound_effect( attacker_wins ? e_sfx::attacker_won
                                   : e_sfx::attacker_lost );
  co_await animate_depixelation(
      attacker_wins ? defender : attacker, dp_anim );
}

// FIXME: Would be nice to make this animation a bit more sophis-
// ticated, but we first need to fix the animation framework in
// this module to be more flexible.
wait<> landview_animate_colony_capture( UnitId   attacker_id,
                                        UnitId   defender_id,
                                        ColonyId colony_id ) {
  co_await landview_animate_attack( attacker_id, defender_id,
                                    /*attacker_wins=*/true,
                                    e_depixelate_anim::death );
  UNWRAP_CHECK(
      direction,
      coord_for_unit( attacker_id )
          ->direction_to(
              colony_from_id( colony_id ).location() ) );
  co_await landview_animate_move( attacker_id, direction );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( blinking_unit, maybe<UnitId> ) {
  using u_i = LandViewUnitActionState::unit_input;
  return g_landview_state.get_if<u_i>().member( &u_i::unit_id );
}

LUA_FN( center_on_blinking_unit, void ) {
  (void)center_on_blinking_unit_if_any();
}

LUA_FN( center_on_tile, void, Coord center ) {
  viewport().center_on_tile( center );
}

LUA_FN( set_zoom, void, double zoom ) {
  viewport().smooth_zoom_target( zoom );
}

} // namespace

} // namespace rn
