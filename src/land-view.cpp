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
#include "colony-view.hpp"
#include "compositor.hpp"
#include "config-files.hpp"
#include "coord.hpp"
#include "cstate.hpp"
#include "fb.hpp"
#include "frame.hpp"
#include "gfx.hpp"
#include "id.hpp"
#include "logging.hpp"
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
#include "tx.hpp"
#include "ustate.hpp"
#include "utype.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "waitable-coro.hpp"
#include "window.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/scope-exit.hpp"

// Rnl
#include "rnl/land-view-impl.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

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
  waitable_promise<LandViewRawInput_t> unit_raw_input_promise;
  queue<LandViewRawInput_t>            raw_input_queue;
  maybe<UnitId>                        last_unit_input;

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
    landview_state         = LandViewState::none{};
    unit_raw_input_promise = {};
    raw_input_queue        = {};
    last_unit_input        = nothing;

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
    function_ref<bool( UnitId )> skip ) {
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
  UnitId blinker_id  = id;
  Coord  blink_coord = coord_for_unit_indirect( blinker_id );
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
  UnitId       mover_id    = id;
  Coord        mover_coord = coord_for_unit_indirect( mover_id );
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
    Coord pixel_coord = to_pixel_coord( covered, coord );
    // FIXME: since colony icons spill over the usual 32x32 tile
    // we need to render colonies that are beyond the `covered`
    // rect.
    if( auto col_id = colony_from_coord( coord );
        col_id.has_value() )
      render_colony( g_texture_viewport, *col_id,
                     pixel_coord - Delta{ 6_w, 6_h } );
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
                  unit_from_id( id ).desc().demoted );
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
  // Start animation.
  while( !depixelate.pixels.empty() ) {
    // FIXME: consider using wait_for_duration here. However,
    // given that the time we're waiting is on the order of a
    // frame at 60 fps, that alone won't work for low frame rates
    // unless we then check how many frames have passed and
    // evolve the animation accordingly.
    co_await 1_frames;
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
                       : Color( 0, 0, 0, 0 );
      set_render_draw_color( color );
      depixelate.tx_depixelate_from.set_render_target();
      ::SDL_RenderDrawPoint( g_renderer, point.x._, point.y._ );
    }
  }
  // End animation.
  UNWRAP_CHECK( it, base::find( SG().unit_animations, id ) );
  SG().unit_animations.erase( it );
}

waitable<> animate_blink( UnitId id ) {
  using namespace std::literals::chrono_literals;
  CHECK( !SG().unit_animations.contains( id ) );
  UnitAnimation::blink& blink =
      SG().unit_animations[id].emplace<UnitAnimation::blink>();
  blink = UnitAnimation::blink{ .visible = false };
  // The unit will always end up visible after we stop since we
  // are going to delete the animation object for this unit,
  // which leaves them visible by default. Do this via RAII be-
  // cause this coroutine will usually be interrupted.
  SCOPE_EXIT( {
    UNWRAP_CHECK( it, base::find( SG().unit_animations, id ) );
    SG().unit_animations.erase( it );
  } );

  // Start animation.
  while( true ) {
    co_await 500ms;
    blink.visible = !blink.visible;
  }
}

waitable<> animate_slide( UnitId id, e_direction d ) {
  CHECK( !SG().unit_animations.contains( id ) );
  Coord                 target = coord_for_unit_indirect( id );
  UnitAnimation::slide& mv =
      SG().unit_animations[id].emplace<UnitAnimation::slide>();
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
  // Start animation.
  while( mv.percent <= 1.0 ) {
    // FIXME: consider using wait_for_duration here. However,
    // given that the time we're waiting is on the order of a
    // frame at 60 fps, that alone won't work for low frame rates
    // unless we then check how many frames have passed and
    // evolve the animation accordingly.
    co_await 1_frames;
    mv.percent_vel.advance( e_push_direction::none );
    mv.percent += mv.percent_vel.to_double();
  }
  // End animation.
  UNWRAP_CHECK( it, base::find( SG().unit_animations, id ) );
  SG().unit_animations.erase( it );
}

waitable<> landview_ensure_unit_visible( UnitId id ) {
  auto coords = coord_for_unit_indirect( id );
  return SG().viewport.ensure_tile_visible_smooth( coords );
}

void center_on_blinking_unit_if_any() {
  using u_i = LandViewState::unit_input;
  auto blinking_unit =
      SG().landview_state.get_if<u_i>().member( &u_i::unit_id );
  if( !blinking_unit ) {
    lg.warn( "There are no units currently asking for orders." );
    return;
  }
  (void)landview_ensure_unit_visible( *blinking_unit );
}

/****************************************************************
** Tile Clicking
*****************************************************************/
// If this is default constructed then it should represent "no
// actions need to be taken."
struct ClickTileActions {
  vector<UnitId> bring_to_front{};
  vector<UnitId> add_to_back{};
};
NOTHROW_MOVE( ClickTileActions );

waitable<maybe<LandViewPlayerInput_t>> ProcessClickTileActions(
    ClickTileActions const& actions ) {
  maybe<LandViewPlayerInput_t> res;
  if( actions.add_to_back.empty() &&
      actions.bring_to_front.empty() )
    co_return res;
  res.emplace();
  auto& change_orders =
      res->emplace<LandViewPlayerInput::change_orders>();
  if( !actions.add_to_back.empty() )
    change_orders.add_to_back = actions.add_to_back;
  if( !actions.bring_to_front.empty() ) {
    // FIXME: maybe this logic should be moved into the turn mod-
    // ule, at which point this will no longer be a coroutine,
    // which it probably shouldn't be.
    auto prioritize = actions.bring_to_front;
    erase_if( prioritize,
              L( unit_from_id( _ ).mv_pts_exhausted() ) );
    auto orig_size = actions.bring_to_front.size();
    auto curr_size = prioritize.size();
    CHECK( curr_size <= orig_size );
    if( curr_size == 0 ) {
      co_await ui::message_box(
          "The selected unit(s) have already moved this turn." );
    } else {
      if( curr_size < orig_size )
        co_await ui::message_box(
            "Some of the selected units have already moved this "
            "turn." );
      change_orders.add_to_front = prioritize;
    }
  }
  co_return res;
}

ClickTileActions ClickTileActionsFromUnitSelections(
    vector<ui::UnitSelection> const& selections,
    bool                             allow_activate ) {
  ClickTileActions result{};
  for( auto const& selection : selections ) {
    auto& sel_unit = unit_from_id( selection.id );
    switch( selection.what ) {
      case ui::e_unit_selection::clear_orders:
        lg.debug( "clearing orders for {}.",
                  debug_string( sel_unit ) );
        sel_unit.clear_orders();
        if( allow_activate )
          result.add_to_back.push_back( selection.id );
        break;
      case ui::e_unit_selection::activate:
        CHECK( allow_activate );
        lg.debug( "activating {}.", debug_string( sel_unit ) );
        // Activation implies also to clear orders if they're
        // not already cleared.
        sel_unit.clear_orders();
        result.bring_to_front.push_back( selection.id );
        break;
    }
  }
  return result;
}

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
waitable<ClickTileActions> click_on_world_tile(
    Coord coord, bool allow_activate ) {
  // First check for colonies.
  if( auto maybe_id = colony_from_coord( coord ); maybe_id ) {
    show_colony_view( *maybe_id );
    co_return {};
  }

  // Now check for units.
  auto const& units = units_from_coord_recursive( coord );
  if( units.size() == 0 ) co_return {};

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

  co_return ClickTileActionsFromUnitSelections( selections,
                                                allow_activate );
}

/****************************************************************
** Input Processor
*****************************************************************/
waitable<LandViewPlayerInput_t> next_player_input_object() {
  while( true ) {
    LandViewRawInput_t raw_input =
        co_await SG().unit_raw_input_promise.waitable();
    // Reset it so that the next one will be inserted.
    SG().unit_raw_input_promise = {};

    switch( raw_input.to_enum() ) {
      using namespace LandViewRawInput;
      case e::orders: {
        co_return LandViewPlayerInput::give_orders{
            .orders = raw_input.get<LandViewRawInput::orders>()
                          .orders };
      }
      case e::tile_click: {
        auto& o = raw_input.get<tile_click>();
        bool  allow_activate =
            SG().landview_state
                .holds<LandViewState::unit_input>();
        ClickTileActions actions = co_await click_on_world_tile(
            o.coord, allow_activate );
        maybe<LandViewPlayerInput_t> input =
            co_await ProcessClickTileActions( actions );
        if( input ) co_return *input;
      }
    }
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

struct LandViewPlane : public Plane {
  LandViewPlane() = default;
  bool covers_screen() const override { return true; }
  void advance_state() override {
    if( !SG().unit_raw_input_promise.has_value() &&
        !SG().raw_input_queue.empty() ) {
      SG().unit_raw_input_promise.set_value(
          SG().raw_input_queue.front() );
      SG().raw_input_queue.pop();
    }
    advance_viewport_state();
  }
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
        switch( key_event.keycode ) {
          case ::SDLK_z:
            SG().viewport.smooth_zoom_target( 1.0 );
            break;
          case ::SDLK_w:
            SG().raw_input_queue.push( LandViewRawInput::orders{
                .orders = orders::wait{} } );
            break;
          case ::SDLK_s:
            SG().raw_input_queue.push( LandViewRawInput::orders{
                .orders = orders::sentry{} } );
            break;
          case ::SDLK_f:
            SG().raw_input_queue.push( LandViewRawInput::orders{
                .orders = orders::fortify{} } );
            break;
          case ::SDLK_b:
            SG().raw_input_queue.push( LandViewRawInput::orders{
                .orders = orders::build{} } );
            break;
          case ::SDLK_c: center_on_blinking_unit_if_any(); break;
          case ::SDLK_d:
            SG().raw_input_queue.push( LandViewRawInput::orders{
                .orders = orders::disband{} } );
            break;
          case ::SDLK_SPACE:
          case ::SDLK_KP_5:
            SG().raw_input_queue.push( LandViewRawInput::orders{
                .orders = orders::forfeight{} } );
            break;
          default:
            handled = e_input_handled::no;
            if( key_event.direction ) {
              SG().raw_input_queue.push(
                  LandViewRawInput::orders{
                      .orders = orders::direction{
                          *key_event.direction } } );
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
          SG().raw_input_queue.push(
              LandViewRawInput::tile_click{ .coord =
                                                *maybe_tile } );
          handled = e_input_handled::yes;
        }
        break;
      }
      default: //
        break;
    }
    return handled;
  }
  Plane::DragInfo can_drag( input::e_mouse_button button,
                            Coord origin ) override {
    if( button == input::e_mouse_button::r &&
        SG().viewport.screen_coord_in_viewport( origin ) )
      return Plane::e_accept_drag::yes;
    return Plane::e_accept_drag::no;
  }
  void on_drag( input::mod_keys const& /*unused*/,
                input::e_mouse_button /*unused*/,
                Coord /*unused*/, Coord prev,
                Coord current ) override {
    SG().viewport.stop_auto_panning();
    // When the mouse drags up, we need to move the viewport
    // center down.
    SG().viewport.pan_by_screen_coords( prev - current );
  }
};

LandViewPlane g_land_view_plane;

} // namespace

Plane* land_view_plane() { return &g_land_view_plane; }

/****************************************************************
** Public API
*****************************************************************/
waitable<LandViewPlayerInput_t> landview_get_next_input(
    UnitId id ) {
  // When we start on a new unit clear the input queue so that
  // commands that were accidentally buffered while controlling
  // the previous unit don't affect this new one, which would al-
  // most certainly not be desirable. Also, we only pan to the
  // unit here because if we did that outside of this if state-
  // ment then the viewport would pan to the blinking unit after
  // the player e.g. clicks on another unit to activate it.
  if( SG().last_unit_input != id ) {
    SG().raw_input_queue        = {};
    SG().unit_raw_input_promise = {};
    co_await landview_ensure_unit_visible( id );
  }
  SG().last_unit_input = id;

  SG().landview_state =
      LandViewState::unit_input{ .unit_id = id };

  LandViewPlayerInput_t res;
  {
    // FIXME: run blinker under a combinator that will automati-
    // cally cancel it without us having to use scopes here.
    waitable<> blinker = animate_blink( id );
    SCOPE_EXIT( blinker.cancel() );
    res = co_await next_player_input_object();
  }

  // Must be last.
  SG().landview_state = LandViewState::none{};
  co_return res;
}

waitable<> landview_animate_move( UnitId      id,
                                  e_direction direction ) {
  co_await landview_ensure_unit_visible( id );
  SG().landview_state =
      LandViewState::unit_move{ .unit_id = id };
  co_await animate_slide( id, direction );
  // Must be last.
  SG().landview_state = LandViewState::none{};
}

waitable<> landview_animate_attack( UnitId attacker,
                                    UnitId defender,
                                    bool   attacker_wins,
                                    e_depixelate_anim dp_anim ) {
  co_await landview_ensure_unit_visible( attacker );
  SG().landview_state = LandViewState::unit_attack{
      .attacker      = attacker,
      .defender      = defender,
      .attacker_wins = attacker_wins };
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
  // Must be last.
  SG().landview_state = LandViewState::none{};
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
  center_on_blinking_unit_if_any();
}

} // namespace

} // namespace rn
