/****************************************************************
* loops.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: 
*
*****************************************************************/
#include "loops.hpp"

#include "movement.hpp"
#include "ownership.hpp"
#include "physics.hpp"
#include "render.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

namespace rn {

namespace {

constexpr double movement_speed = 8.0;
constexpr double zoom_speed = .08;
  
} // namespace

e_orders_loop_result loop_orders( UnitId id ) {
  int frame_rate = 120;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;

  auto& unit = unit_from_id( id );
  auto coords = coords_for_unit( id );

  UnitMoveDesc move_desc;

  viewport::ensure_tile_surroundings_visible( coords );

  long total_frames = 0;
  auto ticks_start_loop = ::SDL_GetTicks();

  long ticks_render = 0;

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    auto ticks_render_start = ::SDL_GetTicks();
    render_world_viewport( id );
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += (ticks_render_end-ticks_render_start);
    total_frames++;

    if( ::SDL_Event event; SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          return e_orders_loop_result::quit;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED:
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case ::SDLK_q:
              { running = false;
              auto ticks_end_loop = ::SDL_GetTicks();
              std::cerr << "average framerate: " << 1000.0*double(total_frames)/(ticks_end_loop-ticks_start_loop) << "\n";
              std::cerr << "average ticks/render: " << double(ticks_render)/total_frames << "\n";
              std::cerr << "render % of frame: " << 100.0*double(ticks_render)/(ticks_end_loop-ticks_start_loop) << "\n";
              return e_orders_loop_result::quit;}
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
            case ::SDLK_w:
              running = false;
              return e_orders_loop_result::wait;
            case ::SDLK_SPACE:
              running = false;
              unit.forfeight_mv_points();
              return e_orders_loop_result::moved;
            case ::SDLK_LEFT:
              move_desc = move_consequences(
                  id, coords.moved( direction::w ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
            case ::SDLK_RIGHT:
              move_desc = move_consequences(
                  id, coords.moved( direction::e ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
            case ::SDLK_DOWN:
              move_desc = move_consequences(
                  id, coords.moved( direction::s ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
            case ::SDLK_UP:
              move_desc = move_consequences(
                  id, coords.moved( direction::n ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 ) {
            viewport::scale_zoom( 0.98 );
          }
          if( event.wheel.y > 0 ) {
            viewport::scale_zoom( 1.02 );
          }
          break;
        default:
          break;
      }
    }
    auto ticks_end = ::SDL_GetTicks();
    auto delta = ticks_end-ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  // If we're here then the unit needs to be physically moved.
  move_unit_to( id, move_desc.coords );
  return e_orders_loop_result::moved;
}

e_eot_loop_result loop_eot() {
  int frame_rate = 120;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;
  e_eot_loop_result result = e_eot_loop_result::none;

  long total_frames = 0;
  auto ticks_start_loop = ::SDL_GetTicks();

  long ticks_render = 0;

  double zoom_accel      = 0.2*zoom_speed;
  double zoom_accel_drag = 0.05*zoom_speed;
  double pan_accel       = 0.2*movement_speed;
  double pan_accel_drag  = 0.1*movement_speed;

  DissipativeVelocity mv_vel_x(
      /*min_velocity=*/-movement_speed,
      /*max_velocity=*/movement_speed,
      /*initial_vel=*/0,
      /*mag_acceleration=*/pan_accel,
      /*mag_drag_acceleration=*/pan_accel_drag );
  DissipativeVelocity mv_vel_y(
      /*min_velocity=*/-movement_speed,
      /*max_velocity=*/movement_speed,
      /*initial_vel=*/0,
      /*mag_acceleration=*/pan_accel,
      /*mag_drag_acceleration=*/pan_accel_drag );
  DissipativeVelocity zoom_vel(
      /*min_velocity=*/-zoom_speed,
      /*max_velocity=*/zoom_speed,
      /*initial_vel=*/0,
      /*mag_acceleration=*/zoom_accel,
      /*mag_drag_acceleration=*/zoom_accel_drag );

  enum class e_zoom_event {
    none, in, out
  };

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    double zoom_factor07 = pow( viewport::get_scale_zoom(), 0.7 );
    double zoom_factor15 = pow( viewport::get_scale_zoom(), 1.5 );
    pan_accel       = 0.2*movement_speed;
    pan_accel_drag  = 0.1*movement_speed;
    pan_accel_drag = pan_accel_drag / pow( viewport::get_scale_zoom(), .75 );
    pan_accel = pan_accel_drag + (pan_accel-pan_accel_drag)/zoom_factor15;
    mv_vel_x.set_accelerations( pan_accel, pan_accel_drag );
    mv_vel_y.set_accelerations( pan_accel, pan_accel_drag );
    mv_vel_x.set_bounds( -movement_speed/zoom_factor07,
                          movement_speed/zoom_factor07 );
    mv_vel_y.set_bounds( -movement_speed/zoom_factor07,
                          movement_speed/zoom_factor07 );

    auto ticks_start = ::SDL_GetTicks();

    total_frames++;

    auto ticks_render_start = ::SDL_GetTicks();
    render_world_viewport();
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += (ticks_render_end-ticks_render_start);

    e_push_direction zoom_direction = e_push_direction::none;

    ::SDL_Event event;
    while( SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          result = e_eot_loop_result::quit;
          break;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED:
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case ::SDLK_q:
              { running = false;
              result = e_eot_loop_result::quit;
              auto ticks_end_loop = ::SDL_GetTicks();
              std::cerr << "average framerate: " << 1000.0*double(total_frames)/(ticks_end_loop-ticks_start_loop) << "\n";
              std::cerr << "average ticks/render: " << double(ticks_render)/total_frames << "\n";
              std::cerr << "render % of frame: " << 100.0*double(ticks_render)/(ticks_end_loop-ticks_start_loop) << "\n";
              break;}
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            zoom_direction = e_push_direction::negative;
          if( event.wheel.y > 0 )
            zoom_direction = e_push_direction::positive;
          break;
        default:
          break;
      }
    }

    auto const* state = ::SDL_GetKeyboardState( NULL );

    mv_vel_x.advance(
        state[::SDL_SCANCODE_LEFT]  ? e_push_direction::negative
      : state[::SDL_SCANCODE_RIGHT] ? e_push_direction::positive
      : e_push_direction::none );

    mv_vel_y.advance(
        state[::SDL_SCANCODE_UP]   ? e_push_direction::negative
      : state[::SDL_SCANCODE_DOWN] ? e_push_direction::positive
      : e_push_direction::none );

    zoom_vel.advance( zoom_direction );

    viewport::pan( 0, mv_vel_x, false );
    viewport::pan( mv_vel_y, 0, false );
    viewport::scale_zoom( 1.0+zoom_vel );

    auto ticks_end = ::SDL_GetTicks();
    auto delta = ticks_end-ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  return result;
}

} // namespace rn
