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
#include "co-waitable.hpp"
#include "compositor.hpp"
#include "config-files.hpp"
#include "coord.hpp"
#include "cstate.hpp"
#include "fb.hpp"
#include "gfx.hpp"
#include "id.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "matrix.hpp"
#include "orders.hpp"
#include "physics.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "sg-macros.hpp"
#include "sound.hpp"
#include "terrain.hpp"
#include "text.hpp"
#include "tx.hpp"
#include "ustate.hpp"
#include "utype.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "window.hpp"

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
#include "rds/land-view-impl.hpp"

// Revolution Now (config)
#include "../config/rcl/land-view.inl"
#include "../config/rcl/rn.inl"

// Flatbuffers
#include "fb/sg-land-view_generated.h"

// C++ standard library
#include <chrono>
#include <queue>
#include <unordered_map>

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( LandView );

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

co::stream<RawInput>    g_raw_input_stream;
co::stream<PlayerInput> g_translated_input_stream;

bool g_needs_scroll_to_unit_on_input = true;

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( LandView ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( LandView,
  ( SmoothViewport, viewport ));
  // clang-format on

 public:
  // Non-serialized fields.
  unordered_map<UnitId, UnitAnimation_t> unit_animations;
  LandViewState_t landview_state = LandViewState::none{};
  maybe<UnitId>   last_unit_input;

 private:
  SAVEGAME_FRIENDS( LandView );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    UNWRAP_CHECK(
        viewport_rect_pixels,
        compositor::section( compositor::e_section::viewport ) );
    // This call is needed after construction to initialize the
    // invariants. It is expected that the parameters of this
    // function will not depend on any other deserialized data,
    // but only on data available after the init routines run.
    viewport.advance_state( viewport_rect_pixels,
                            world_size_tiles() );

    // Initialize general global data.
    unit_animations.clear();
    landview_state  = LandViewState::none{};
    last_unit_input = nothing;
    g_raw_input_stream.reset();
    g_translated_input_stream.reset();
    g_needs_scroll_to_unit_on_input = true;

    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return valid; }
};
SAVEGAME_IMPL( LandView );

/****************************************************************
** Land-View Rendering
*****************************************************************/
Coord to_pixel_coord( Rect const& covered, Coord tile_coord ) {
  Coord pixel_coord =
      Coord{} + ( tile_coord - covered.upper_left() );
  pixel_coord *= g_tile_scale;
  return pixel_coord;
};

void render_units_on_square(
    Rect const& covered, Coord coord,
    base::function_ref<bool( UnitId )> skip ) {
  Coord pixel_coord = to_pixel_coord( covered, coord );
  // TODO: When there are multiple units on a square, just
  // render one (which one?) and then render multiple flags (s-
  // tacked) to indicate that visually.
  for( auto id : units_from_coord( coord ) ) {
    if( skip( id ) ) continue;
    render_unit( g_texture_viewport, id, pixel_coord,
                 /*with_icon=*/true );
  }
}

void render_units_on_square( Rect const& covered, Coord coord ) {
  render_units_on_square( covered, coord,
                          []( UnitId ) { return false; } );
}

void render_units_default( Rect const& covered ) {
  for( auto coord : covered ) {
    if( colony_from_coord( coord ).has_value() ) continue;
    render_units_on_square( covered, coord );
  }
}

void render_units_blink( Rect const& covered, UnitId id,
                         bool visible ) {
  UnitId blinker_id = id;
  Coord  blink_coord =
      coord_for_unit_indirect_or_die( blinker_id );
  for( auto coord : covered ) {
    if( coord == blink_coord ) continue;
    if( colony_from_coord( coord ).has_value() ) continue;
    render_units_on_square( covered, coord );
  }
  // Now render the blinking unit.
  Coord pixel_coord = to_pixel_coord( covered, blink_coord );
  if( visible )
    render_unit( g_texture_viewport, blinker_id, pixel_coord,
                 /*with_icon=*/true );
}

void render_units_during_slide(
    Rect const& covered, UnitId id, maybe<UnitId> target_unit,
    UnitAnimation::slide const& slide ) {
  UnitId mover_id   = id;
  Coord mover_coord = coord_for_unit_indirect_or_die( mover_id );
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
    render_units_on_square( covered, coord, skip );
  }
  // Now render the sliding unit.
  Delta delta = slide.target - mover_coord;
  CHECK( -1 <= delta.w && delta.w <= 1 );
  CHECK( -1 <= delta.h && delta.h <= 1 );
  delta *= g_tile_scale;
  Delta pixel_delta{ W( int( delta.w._ * slide.percent ) ),
                     H( int( delta.h._ * slide.percent ) ) };
  Coord pixel_coord = to_pixel_coord( covered, mover_coord );
  pixel_coord += pixel_delta;
  render_unit( g_texture_viewport, mover_id, pixel_coord,
               /*with_icon=*/true );
}

void render_units_during_depixelate(
    Rect const& covered, UnitId depixelate_id,
    UnitAnimation::depixelate const& dp_anim ) {
  // Need the multi_ownership version because we could be depixe-
  // lating a colonist that is owned by a colony, which happens
  // when the colony is captured.
  UNWRAP_CHECK( depixelate_coord, coord_for_unit_multi_ownership(
                                      depixelate_id ) );
  // First render all units other than the depixelating unit and
  // other than units on colony squares.
  for( auto coord : covered ) {
    if( coord == depixelate_coord ) continue;
    if( colony_from_coord( coord ).has_value() ) continue;
    render_units_on_square( covered, coord,
                            /*skip=*/[&]( UnitId id ) {
                              return id == depixelate_id;
                            } );
  }
  // Now render the depixelating unit.
  ::SDL_SetRenderDrawBlendMode( g_renderer,
                                ::SDL_BLENDMODE_BLEND );
  Coord pixel_coord =
      to_pixel_coord( covered, depixelate_coord );
  copy_texture( dp_anim.tx_depixelate_from, g_texture_viewport,
                pixel_coord );
}

void render_colonies( Rect const& covered ) {
  for( auto coord : covered ) {
    Coord tile_coord = to_pixel_coord( covered, coord );
    // FIXME: since colony icons spill over the usual 32x32 tile
    // we need to render colonies that are beyond the `covered`
    // rect.
    if( auto col_id = colony_from_coord( coord );
        col_id.has_value() ) {
      Colony const& colony = colony_from_id( *col_id );
      Coord         colony_sprite_upper_left =
          tile_coord - Delta{ 6_w, 6_h };
      render_colony( g_texture_viewport, *col_id,
                     colony_sprite_upper_left );
      Coord name_coord =
          tile_coord +
          config_land_view.colonies.colony_name_offset;
      copy_texture(
          render_text_markup(
              config_land_view.colonies.colony_name_font,
              TextMarkupInfo{
                  .shadowed_text_color   = gfx::pixel::white(),
                  .shadowed_shadow_color = gfx::pixel::black() },
              fmt::format( "@[S]{}@[]", colony.name() ) ),
          g_texture_viewport, name_coord );
    }
  }
}

// Units (rendering strategy depends on land view state).
void render_units( Rect const& covered ) {
  // The land view state should be set first, then the animation
  // state; though occasionally we are in a frame where the land
  // view state has changed but the animation state has not been
  // set yet.
  if( SG().unit_animations.empty() ) {
    render_units_default( covered );
    return;
  }

  switch( SG().landview_state.to_enum() ) {
    using namespace LandViewState;
    case e::none: {
      // In this case the global unit animations should always be
      // empty, in which case the if statement above should have
      // caught it.
      SHOULD_NOT_BE_HERE;
    }
    case e::unit_input: {
      auto& o = SG().landview_state.get<unit_input>();
      CHECK( SG().unit_animations.size() == 1 );
      UNWRAP_CHECK(
          animation,
          base::lookup( SG().unit_animations, o.unit_id ) );
      ASSIGN_CHECK_V( blink_anim, animation,
                      UnitAnimation::blink );
      render_units_blink( covered, o.unit_id,
                          blink_anim.visible );
      break;
    }
    case e::unit_move: {
      auto& o = SG().landview_state.get<unit_move>();
      CHECK( SG().unit_animations.size() == 1 );
      UNWRAP_CHECK(
          animation,
          base::lookup( SG().unit_animations, o.unit_id ) );
      ASSIGN_CHECK_V( slide_anim, animation,
                      UnitAnimation::slide );
      render_units_during_slide( covered, o.unit_id,
                                 /*target_unit=*/nothing,
                                 slide_anim );
      break;
    }
    case e::unit_attack: {
      auto& o = SG().landview_state.get<unit_attack>();
      CHECK( SG().unit_animations.size() == 1 );
      UnitId anim_id = SG().unit_animations.begin()->first;
      UnitAnimation_t const& animation =
          SG().unit_animations.begin()->second;
      if( auto slide_anim =
              animation.get_if<UnitAnimation::slide>() ) {
        CHECK( anim_id == o.attacker );
        render_units_during_slide( covered, o.attacker,
                                   o.defender, *slide_anim );
        break;
      }
      if( auto dp_anim =
              animation.get_if<UnitAnimation::depixelate>() ) {
        render_units_during_depixelate( covered, anim_id,
                                        *dp_anim );
        break;
      }
      FATAL(
          "Unit animation not found for either slide or "
          "depixelate." );
      break;
    }
  }
}

void render_land_view() {
  g_texture_viewport.set_render_target();
  auto covered = SG().viewport.covered_tiles();
  render_terrain( covered, g_texture_viewport, Coord{} );
  render_colonies( covered );
  render_units( covered );
}

/****************************************************************
** Animations
*****************************************************************/
waitable<> animate_depixelation( UnitId            id,
                                 e_depixelate_anim dp_anim ) {
  CHECK( !SG().unit_animations.contains( id ) );
  UnitAnimation::depixelate& depixelate =
      SG().unit_animations[id]
          .emplace<UnitAnimation::depixelate>();
  SCOPE_EXIT( {
    UNWRAP_CHECK( it, base::find( SG().unit_animations, id ) );
    SG().unit_animations.erase( it );
  } );
  depixelate.pixels.assign( g_tile_rect.begin(),
                            g_tile_rect.end() );
  rng::shuffle( depixelate.pixels );
  depixelate.tx_depixelate_from = create_texture( g_tile_delta );
  clear_texture_transparent( depixelate.tx_depixelate_from );
  render_unit( depixelate.tx_depixelate_from, id, Coord{},
               /*with_icon=*/true );
  // Now, if we are depixelating to another unit then we will set
  // that process up.
  if( dp_anim == e_depixelate_anim::demote ) {
    UNWRAP_CHECK( demoted_type,
                  unit_from_id( id ).demoted_type() );
    auto tx = Texture::create( g_tile_delta );
    clear_texture_transparent( tx );
    auto& from_unit = unit_from_id( id );
    // Render the target unit with no nationality icon, then
    // render the nationality icon of the unit being depixe-
    // lated so that it doesn't appear to change during the an-
    // imation.
    if( !from_unit.desc().nat_icon_front ) {
      render_unit( tx, demoted_type, Coord{} );
      render_nationality_icon( tx, id, Coord{} );
    } else {
      render_nationality_icon( tx, id, Coord{} );
      render_unit( tx, demoted_type, Coord{} );
    }
    depixelate.demoted_pixels = tx.pixels();
  }
  AnimThrottler throttle( kAlmostStandardFrame );
  while( !depixelate.pixels.empty() ) {
    co_await throttle();
    int to_depixelate =
        std::min( config_rn.depixelate_pixels_per_frame,
                  int( depixelate.pixels.size() ) );
    vector<Coord> new_non_pixels;
    for( int i = 0; i < to_depixelate; ++i ) {
      auto next_coord = depixelate.pixels.back();
      depixelate.pixels.pop_back();
      new_non_pixels.push_back( next_coord );
    }
    ::SDL_SetRenderDrawBlendMode( g_renderer,
                                  ::SDL_BLENDMODE_NONE );
    for( auto point : new_non_pixels ) {
      auto color = depixelate.demoted_pixels.size().area() > 0
                       ? depixelate.demoted_pixels[point]
                       : gfx::pixel( 0, 0, 0, 0 );
      set_render_draw_color( color );
      depixelate.tx_depixelate_from.set_render_target();
      ::SDL_RenderDrawPoint( g_renderer, point.x._, point.y._ );
    }
  }
}

waitable<> animate_blink( UnitId id ) {
  using namespace std::literals::chrono_literals;
  CHECK( !SG().unit_animations.contains( id ) );
  UnitAnimation::blink& blink =
      SG().unit_animations[id].emplace<UnitAnimation::blink>();
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
    UNWRAP_CHECK( it, base::find( SG().unit_animations, id ) );
    SG().unit_animations.erase( it );
  } );

  AnimThrottler throttle( 500ms, /*initial_delay=*/true );
  while( true ) {
    co_await throttle();
    blink.visible = !blink.visible;
  }
}

waitable<> animate_slide( UnitId id, e_direction d ) {
  CHECK( !SG().unit_animations.contains( id ) );
  Coord target = coord_for_unit_indirect_or_die( id );
  UnitAnimation::slide& mv =
      SG().unit_animations[id].emplace<UnitAnimation::slide>();
  SCOPE_EXIT( {
    UNWRAP_CHECK( it, base::find( SG().unit_animations, id ) );
    SG().unit_animations.erase( it );
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

waitable<> center_on_blinking_unit_if_any() {
  using u_i = LandViewState::unit_input;
  auto blinking_unit =
      SG().landview_state.get_if<u_i>().member( &u_i::unit_id );
  if( !blinking_unit ) {
    lg.warn( "There are no units currently asking for orders." );
    return make_waitable<>();
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
waitable<vector<LandViewPlayerInput_t>> click_on_world_tile(
    Coord coord ) {
  vector<LandViewPlayerInput_t> res;
  auto add = [&res]<typename T>( T t ) -> T& {
    res.push_back( std::move( t ) );
    return res.back().get<T>();
  };

  bool allow_activate =
      SG().landview_state.holds<LandViewState::unit_input>();

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
waitable<> raw_input_translator() {
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

waitable<LandViewPlayerInput_t> next_player_input_object() {
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

  SG().viewport.advance_state( viewport_rect_pixels,
                               world_size_tiles() );

  // TODO: should only do the following when the viewport has
  // input focus.
  auto const* __state = ::SDL_GetKeyboardState( nullptr );

  // Returns true if key is pressed.
  auto state = [__state]( ::SDL_Scancode code ) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return __state[code] != 0;
  };

  if( state( ::SDL_SCANCODE_LSHIFT ) ) {
    SG().viewport.set_x_push(
        state( ::SDL_SCANCODE_A )   ? e_push_direction::negative
        : state( ::SDL_SCANCODE_D ) ? e_push_direction::positive
                                    : e_push_direction::none );
    // y motion
    SG().viewport.set_y_push(
        state( ::SDL_SCANCODE_W )   ? e_push_direction::negative
        : state( ::SDL_SCANCODE_S ) ? e_push_direction::positive
                                    : e_push_direction::none );

    if( state( ::SDL_SCANCODE_A ) || state( ::SDL_SCANCODE_D ) ||
        state( ::SDL_SCANCODE_W ) || state( ::SDL_SCANCODE_S ) )
      SG().viewport.stop_auto_panning();
  }
}

maybe<orders_t> try_orders_from_lua( int keycode, bool ctrl_down,
                                     bool shf_down ) {
  lua::state& st = lua_global_state();

  lua::any lua_orders = st["land-view"]["key_to_orders"](
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
  void advance_state() override { advance_viewport_state(); }
  void draw( Texture& tx ) const override {
    render_land_view();
    clear_texture_black( tx );
    copy_texture_stretch( g_texture_viewport, tx,
                          SG().viewport.rendering_src_rect(),
                          SG().viewport.rendering_dest_rect() );
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
        SG().viewport.stop_auto_zoom();
        SG().viewport.stop_auto_panning();
        SG().viewport.smooth_zoom_target(
            SG().viewport.get_zoom() * zoom_in_factor );
      };
      return handler;
    }
    if( item == e_menu_item::zoom_out ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        SG().viewport.stop_auto_zoom();
        SG().viewport.stop_auto_panning();
        SG().viewport.smooth_zoom_target(
            SG().viewport.get_zoom() * zoom_out_factor );
      };
      return handler;
    }
    if( item == e_menu_item::restore_zoom ) {
      if( SG().viewport.get_zoom() == 1.0 ) return nothing;
      static Plane::MenuClickHandler handler = [] {
        SG().viewport.smooth_zoom_target( 1.0 );
      };
      return handler;
    }
    if( item == e_menu_item::find_blinking_unit ) {
      if( !SG().landview_state
               .holds<LandViewState::unit_input>() )
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
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
            if( SG().viewport.get_zoom() < 1.0 )
              SG().viewport.smooth_zoom_target( 1.0 );
            else if( SG().viewport.get_zoom() < 1.5 )
              SG().viewport.smooth_zoom_target( 2.0 );
            else
              SG().viewport.smooth_zoom_target( 1.0 );
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
        if( SG().viewport.screen_coord_in_viewport( val.pos ) ) {
          if( val.wheel_delta < 0 )
            SG().viewport.set_zoom_push(
                e_push_direction::negative, nothing );
          if( val.wheel_delta > 0 )
            SG().viewport.set_zoom_push(
                e_push_direction::positive, val.pos );
          // A user zoom request halts any auto zooming that
          // may currently be happening.
          SG().viewport.stop_auto_zoom();
          SG().viewport.stop_auto_panning();
          handled = e_input_handled::yes;
        }
        break;
      }
      case input::e_input_event::mouse_button_event: {
        auto& val = event.get<input::mouse_button_event_t>();
        if( val.buttons != input::e_mouse_button_event::left_up )
          break;
        if( auto maybe_tile =
                SG().viewport.screen_pixel_to_world_tile(
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
  maybe<waitable<>> drag_thread;
  bool              drag_finished = true;

  waitable<> dragging( input::e_mouse_button /*button*/,
                       Coord /*origin*/ ) {
    SCOPE_EXIT( drag_finished = true );
    while( maybe<DragUpdate> d = co_await drag_stream.next() )
      SG().viewport.pan_by_screen_coords( d->prev - d->current );
  }

  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( !drag_finished ) return Plane::e_accept_drag::swallow;
    if( button == input::e_mouse_button::r &&
        SG().viewport.screen_coord_in_viewport( origin ) ) {
      SG().viewport.stop_auto_panning();
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

waitable<> landview_ensure_visible( Coord const& coord ) {
  return SG().viewport.ensure_tile_visible_smooth( coord );
}

waitable<> landview_ensure_visible( UnitId id ) {
  // Need multi-ownership variant because sometimes the unit in
  // question is a worker in a colony, as can happen if we are
  // attacking an undefended colony.
  UNWRAP_CHECK( coord, coord_for_unit_multi_ownership( id ) );
  return landview_ensure_visible( coord );
}

waitable<LandViewPlayerInput_t> landview_get_next_input(
    UnitId id ) {
  // When we start on a new unit clear the input queue so that
  // commands that were accidentally buffered while controlling
  // the previous unit don't affect this new one, which would al-
  // most certainly not be desirable. Also, we only pan to the
  // unit here because if we did that outside of this if state-
  // ment then the viewport would pan to the blinking unit after
  // the player e.g. clicks on another unit to activate it.
  if( SG().last_unit_input != id )
    g_needs_scroll_to_unit_on_input = true;

  // This might be true either because we started a new turn, or
  // because of the above assignment.
  if( g_needs_scroll_to_unit_on_input )
    co_await landview_ensure_visible( id );
  g_needs_scroll_to_unit_on_input = false;

  if( SG().last_unit_input != id )
    landview_reset_input_buffers();
  SG().last_unit_input = id;

  SCOPED_SET_AND_RESTORE(
      SG().landview_state,
      LandViewState::unit_input{ .unit_id = id } );

  // Run the blinker while waiting for user input.
  co_return co_await co::background( next_player_input_object(),
                                     animate_blink( id ) );
}

waitable<LandViewPlayerInput_t> landview_eot_get_next_input() {
  SG().last_unit_input = nothing;
  SG().landview_state  = LandViewState::none{};
  return next_player_input_object();
}

waitable<> landview_animate_move( UnitId      id,
                                  e_direction direction ) {
  // Ensure that both src and dst squares are visible.
  Coord src = coord_for_unit_indirect_or_die( id );
  Coord dst = src.moved( direction );
  co_await landview_ensure_visible( src );
  co_await landview_ensure_visible( dst );
  SCOPED_SET_AND_RESTORE(
      SG().landview_state,
      LandViewState::unit_move{ .unit_id = id } );
  play_sound_effect( e_sfx::move );
  co_await animate_slide( id, direction );
}

waitable<> landview_animate_attack( UnitId attacker,
                                    UnitId defender,
                                    bool   attacker_wins,
                                    e_depixelate_anim dp_anim ) {
  co_await landview_ensure_visible( defender );
  co_await landview_ensure_visible( attacker );
  auto new_state = LandViewState::unit_attack{
      .attacker      = attacker,
      .defender      = defender,
      .attacker_wins = attacker_wins };
  SCOPED_SET_AND_RESTORE( SG().landview_state, new_state );
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
waitable<> landview_animate_colony_capture(
    UnitId attacker_id, UnitId defender_id,
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
  using u_i = LandViewState::unit_input;
  return SG().landview_state.get_if<u_i>().member(
      &u_i::unit_id );
}

LUA_FN( center_on_blinking_unit, void ) {
  (void)center_on_blinking_unit_if_any();
}

} // namespace

} // namespace rn
