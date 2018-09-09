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
#include "render.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

namespace rn {

namespace {

double movement_speed = 8.0;
double zoom_delta = .08;
  
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

  double mv_accel_x      = 0.1;
  double mv_accel_y      = 0.1;
  double zoom_accel      = 0.2;
  double zoom_accel_drag = 0.05;
  double mv_vel_x        = 0.0;
  double mv_vel_y        = 0.0;
  double zoom_vel        = 0.0;

  enum class e_zoom_event {
    none, in, out
  };

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    total_frames++;

    auto ticks_render_start = ::SDL_GetTicks();
    render_world_viewport();
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += (ticks_render_end-ticks_render_start);

    e_zoom_event zoom_event = e_zoom_event::none;

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
            zoom_event = e_zoom_event::out;
          if( event.wheel.y > 0 )
            zoom_event = e_zoom_event::in;
          break;
        default:
          break;
      }
    }
    auto const* state = ::SDL_GetKeyboardState( NULL );
    if( state[::SDL_SCANCODE_LEFT])
      mv_vel_x = (mv_vel_x <= -1.0) ? -1.0 : (mv_vel_x-mv_accel_x);
    if( state[::SDL_SCANCODE_RIGHT])
      mv_vel_x = (mv_vel_x >=  1.0) ?  1.0 : (mv_vel_x+mv_accel_x);
    if( state[::SDL_SCANCODE_UP])
      mv_vel_y = (mv_vel_y <= -1.0) ? -1.0 : (mv_vel_y-mv_accel_y);
    if( state[::SDL_SCANCODE_DOWN])
      mv_vel_y = (mv_vel_y >=  1.0) ?  1.0 : (mv_vel_y+mv_accel_y);

    if( !state[::SDL_SCANCODE_LEFT] && !state[::SDL_SCANCODE_RIGHT] ) {
      if( mv_vel_x > 0 ) {
        mv_vel_x -= mv_accel_x;
        if( mv_vel_x < 0 ) mv_vel_x = 0;
      } else if( mv_vel_x < 0 ) {
        mv_vel_x += mv_accel_x;
        if( mv_vel_x > 0 ) mv_vel_x = 0;
      }
    }
    if( !state[::SDL_SCANCODE_UP] && !state[::SDL_SCANCODE_DOWN] ) {
      if( mv_vel_y > 0 ) {
        mv_vel_y -= mv_accel_y;
        if( mv_vel_y < 0 ) mv_vel_y = 0;
      } else if( mv_vel_y < 0 ) {
        mv_vel_y += mv_accel_y;
        if( mv_vel_y > 0 ) mv_vel_y = 0;
      }
    }

    viewport::pan( 0, movement_speed*mv_vel_x, false );
    viewport::pan( movement_speed*mv_vel_y, 0, false );

    switch( zoom_event ) {
      case e_zoom_event::out:
        zoom_vel = (zoom_vel <= -1.0) ? -1.0 : (zoom_vel-zoom_accel);
        break;
      case e_zoom_event::in:
        zoom_vel = (zoom_vel >=  1.0) ?  1.0 : (zoom_vel+zoom_accel);
        break;
      case e_zoom_event::none:
        if( zoom_vel > 0 ) {
          zoom_vel -= zoom_accel_drag;
          if( zoom_vel < 0 ) zoom_vel = 0;
        } else if( zoom_vel < 0 ) {
          zoom_vel += zoom_accel_drag;
          if( zoom_vel > 0 ) zoom_vel = 0;
        }
        break;
    };

    viewport::scale_zoom( 1.0+zoom_delta*zoom_vel );

    auto ticks_end = ::SDL_GetTicks();
    auto delta = ticks_end-ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  return result;
}

} // namespace rn
