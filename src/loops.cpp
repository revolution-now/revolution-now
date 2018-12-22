/****************************************************************
**loops.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description:
*
*****************************************************************/
#include "loops.hpp"

// Revolution Now
#include "movement.hpp"
#include "ownership.hpp"
#include "physics.hpp"
#include "render.hpp"
#include "sdl-util.hpp"
#include "util.hpp"
#include "viewport.hpp"

// base-util
#include "base-util/variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn {

constexpr int frame_rate{60};

namespace {

absl::flat_hash_map<::SDL_Keycode, direction> nav_keys{
    {::SDLK_LEFT, direction::w},  {::SDLK_RIGHT, direction::e},
    {::SDLK_DOWN, direction::s},  {::SDLK_UP, direction::n},
    {::SDLK_KP_4, direction::w},  {::SDLK_KP_6, direction::e},
    {::SDLK_KP_2, direction::s},  {::SDLK_KP_8, direction::n},
    {::SDLK_KP_7, direction::nw}, {::SDLK_KP_9, direction::ne},
    {::SDLK_KP_1, direction::sw}, {::SDLK_KP_3, direction::se},
};

} // namespace

e_orders_loop_result loop_orders(
    UnitId id, function<void( UnitId )> const& prioritize ) {
  constexpr int millis_per_second{1000};
  unsigned int  frame_length_millis =
      millis_per_second / frame_rate;

  bool running = true;

  auto&      unit   = unit_from_id( id );
  auto const coords = coords_for_unit( id );

  UnitMoveDesc move_desc;

  viewport().ensure_tile_surroundings_visible( coords );

  long total_frames     = 0;
  auto ticks_start_loop = ::SDL_GetTicks();

  long ticks_render = 0;

  RenderStacker push_renderer(
      [id] { render_world_viewport( id ); } );

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    auto ticks_render_start = ::SDL_GetTicks();
    render_all();
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += ( ticks_render_end - ticks_render_start );
    total_frames++;

    e_push_direction zoom_direction = e_push_direction::none;

    ::SDL_Event event;
    while( SDL_PollEvent( &event ) != 0 ) {
      if( !running )
        // This check is needed so that if the player hits two
        // keys at once and the first one to be processed in
        // the switch statement leads to a valid move but the
        // second one does not then we lose the move_desc for
        // the valid move, so we need to break as soon as one
        // event (with a successful move) tells us to.
        break;
      switch( event.type ) {
        case SDL_QUIT: return e_orders_loop_result::quit;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event ) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED: break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case ::SDLK_q: {
              auto ticks_end_loop = ::SDL_GetTicks();
              cerr << "average framerate: "
                   << double( millis_per_second ) *
                          double( total_frames ) /
                          ( ticks_end_loop - ticks_start_loop )
                   << "\n";
              cerr << "average ticks/render: "
                   << double( ticks_render ) / total_frames
                   << "\n";
              cerr << "render % of frame: "
                   << 100.0 * double( ticks_render ) /
                          ( ticks_end_loop - ticks_start_loop )
                   << "\n";
              return e_orders_loop_result::quit;
            }
            case ::SDLK_F11: toggle_fullscreen(); break;
            case ::SDLK_t: return e_orders_loop_result::wait;
            case ::SDLK_SPACE:
            case ::SDLK_KP_5:
              unit.forfeight_mv_points();
              return e_orders_loop_result::moved;
            default:
              auto maybe_direction =
                  val_safe( nav_keys, event.key.keysym.sym );
              if( maybe_direction ) {
                // In case the player has scrolled away from the
                // unit in question.
                viewport().ensure_tile_surroundings_visible(
                    coords );
                move_desc = move_consequences(
                    id, coords.moved( *maybe_direction ) );
                // Any confirmation or messages that the player
                // needs to get prior to this move (or in re-
                // sponse to an illegal move) are done in this
                // function, which then returns true if the move
                // can proceed. In order for the move to proceed
                // we must have both move_desc.can_move() == true
                // and any queries to the player result in the
                // player confirming the move, if applicable.
                running = !confirm_move( move_desc );
              }
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            zoom_direction = e_push_direction::negative;
          if( event.wheel.y > 0 )
            zoom_direction = e_push_direction::positive;
          break;
        default: break;
      }
    }

    auto const* __state = ::SDL_GetKeyboardState( nullptr );

    // Returns true if key is pressed.
    auto state = [__state]( ::SDL_Scancode code ) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      return __state[code] != 0;
    };

    viewport().advance(
        // x motion
        state( ::SDL_SCANCODE_A )
            ? e_push_direction::negative
            : state( ::SDL_SCANCODE_D )
                  ? e_push_direction::positive
                  : e_push_direction::none,
        // y motion
        state( ::SDL_SCANCODE_W )
            ? e_push_direction::negative
            : state( ::SDL_SCANCODE_S )
                  ? e_push_direction::positive
                  : e_push_direction::none,
        // zoom motion
        zoom_direction );

    auto ticks_end = ::SDL_GetTicks();
    auto delta     = ticks_end - ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  // Check if the unit is physically moving; usually at this
  // point it will be unless it is e.g. a ship offloading units.
  if( coords_for_unit( id ) != move_desc.coords ) {
    viewport().ensure_tile_surroundings_visible(
        move_desc.coords );
    loop_mv_unit( id, move_desc.coords );
  }
  move_unit( id, move_desc );
  for( auto id : move_desc.to_prioritize ) prioritize( id );
  if( util::holds( move_desc.desc, e_unit_mv_good::land_fall ) )
    return e_orders_loop_result::offboard;
  return e_orders_loop_result::moved;
} // namespace rn

e_eot_loop_result loop_eot() {
  constexpr int millis_per_second{1000};
  unsigned int  frame_length_millis =
      millis_per_second / frame_rate;

  bool              running = true;
  e_eot_loop_result result  = e_eot_loop_result::none;

  long total_frames     = 0;
  auto ticks_start_loop = ::SDL_GetTicks();

  long ticks_render = 0;

  RenderStacker push_renderer( [] { render_world_viewport(); } );

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    total_frames++;

    auto ticks_render_start = ::SDL_GetTicks();
    render_all();
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += ( ticks_render_end - ticks_render_start );

    e_push_direction zoom_direction = e_push_direction::none;

    ::SDL_Event event;
    while( SDL_PollEvent( &event ) != 0 ) {
      switch( event.type ) {
        case SDL_QUIT:
          running = false;
          result  = e_eot_loop_result::quit;
          break;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event ) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED: break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case ::SDLK_q: {
              running             = false;
              result              = e_eot_loop_result::quit;
              auto ticks_end_loop = ::SDL_GetTicks();
              cerr << "average framerate: "
                   << double( millis_per_second ) *
                          double( total_frames ) /
                          ( ticks_end_loop - ticks_start_loop )
                   << "\n";
              cerr << "average ticks/render: "
                   << double( ticks_render ) / total_frames
                   << "\n";
              cerr << "render % of frame: "
                   << 100.0 * double( ticks_render ) /
                          ( ticks_end_loop - ticks_start_loop )
                   << "\n";
              break;
            }
            case ::SDLK_F11: toggle_fullscreen(); break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            zoom_direction = e_push_direction::negative;
          if( event.wheel.y > 0 )
            zoom_direction = e_push_direction::positive;
          break;
        default: break;
      }
    }

    auto const* __state = ::SDL_GetKeyboardState( nullptr );

    // Returns true if key is pressed.
    auto state = [__state]( ::SDL_Scancode code ) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      return __state[code] != 0;
    };

    viewport().advance(
        // x motion
        state( ::SDL_SCANCODE_A )
            ? e_push_direction::negative
            : state( ::SDL_SCANCODE_D )
                  ? e_push_direction::positive
                  : e_push_direction::none,
        // y motion
        state( ::SDL_SCANCODE_W )
            ? e_push_direction::negative
            : state( ::SDL_SCANCODE_S )
                  ? e_push_direction::positive
                  : e_push_direction::none,
        // zoom motion
        zoom_direction );

    auto ticks_end = ::SDL_GetTicks();
    auto delta     = ticks_end - ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  return result;
}

void loop_mv_unit( UnitId id, Coord const& target ) {
  constexpr int millis_per_second{1000};
  unsigned int  frame_length_millis =
      millis_per_second / frame_rate;

  constexpr auto min_velocity          = 0;
  constexpr auto max_velocity          = .1;
  constexpr auto initial_velocity      = .1;
  constexpr auto mag_acceleration      = 1;
  constexpr auto mag_drag_acceleration = .004;

  DissipativeVelocity percent_vel(
      /*min_velocity=*/min_velocity,
      /*max_velocity=*/max_velocity,
      /*initial_velocity=*/initial_velocity,
      /*mag_acceleration=*/mag_acceleration, // not relevant
      /*mag_drag_acceleration=*/mag_drag_acceleration );

  double percent = 0;
  bool   running = true;

  // Need to take percent by reference because it will be chang-
  // ing.
  RenderStacker push_renderer( [id, &target, &percent] {
    render_world_viewport_mv_unit( id, target, percent );
  } );

  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    render_all();

    percent_vel.advance( e_push_direction::none );
    percent += percent_vel.to_double();
    if( percent > 1.0 ) running = false;

    auto ticks_end = ::SDL_GetTicks();
    auto delta     = ticks_end - ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
}

} // namespace rn
