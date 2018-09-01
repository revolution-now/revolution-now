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

#include "render.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

namespace rn {

namespace {

double movement_speed = 32.0;
  
} // namespace

k_loop_result loop_eot() {
  int frame_rate = 60;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;
  k_loop_result result = k_loop_result::none;

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    render_world_viewport();

    auto ticks_start = ::SDL_GetTicks();
    if( ::SDL_Event event; SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          result = k_loop_result::quit;
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
              running = false;
              result = k_loop_result::quit;
              break;
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
            case ::SDLK_LEFT:
              viewport_pan( 0, -movement_speed, false );
              break;
            case ::SDLK_RIGHT:
              viewport_pan( 0, movement_speed, false );
              break;
            case ::SDLK_DOWN:
              viewport_pan( movement_speed, 0, false );
              break;
            case ::SDLK_UP:
              viewport_pan( -movement_speed, 0, false );
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            viewport_scale_zoom( 0.98 );
          if( event.wheel.y > 0 )
            viewport_scale_zoom( 1.02 );
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
  return result;
}

} // namespace rn
