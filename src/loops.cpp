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

k_orders_loop_result loop_orders( UnitId id ) {
  int frame_rate = 60;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;
  k_orders_loop_result result;

  auto coords = coords_for_unit( id );

  enum class k_unit_mv_result {
    none,
    wait,
    pass,
    move
  };
  k_unit_mv_result mv_result = k_unit_mv_result::none;

  // This will only be relevant if mv_result == move;
  UnitMoveDesc move_desc;

  viewport::ensure_tile_surroundings_visible( coords );

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    render_world_viewport( id );

    auto ticks_start = ::SDL_GetTicks();
    if( ::SDL_Event event; SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          result = k_orders_loop_result::quit;
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
              result = k_orders_loop_result::quit;
              break;
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
            case ::SDLK_w:
              running = false;
              mv_result = k_unit_mv_result::wait;
              result = k_orders_loop_result::wait;
              break;
            case ::SDLK_SPACE:
              running = false;
              forfeight_mv_points( id );
              mv_result = k_unit_mv_result::pass;
              break;
            case ::SDLK_LEFT:
              move_desc = move_consequences(
                  id, coords.moved( direction::w ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                mv_result = k_unit_mv_result::move;
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
                mv_result = k_unit_mv_result::move;
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
                mv_result = k_unit_mv_result::move;
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
                mv_result = k_unit_mv_result::move;
                running = false;
              }
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            viewport::scale_zoom( 0.98 );
          if( event.wheel.y > 0 )
            viewport::scale_zoom( 1.02 );
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
  if( mv_result == k_unit_mv_result::move ) {
    move_unit_to( id, move_desc.coords );
    result = k_orders_loop_result::moved;
  }
  return result;
}

k_eot_loop_result loop_eot() {
  int frame_rate = 60;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;
  k_eot_loop_result result = k_eot_loop_result::none;

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
          result = k_eot_loop_result::quit;
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
              result = k_eot_loop_result::quit;
              break;
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
            case ::SDLK_LEFT:
              viewport::pan( 0, -movement_speed, false );
              break;
            case ::SDLK_RIGHT:
              viewport::pan( 0, movement_speed, false );
              break;
            case ::SDLK_DOWN:
              viewport::pan( movement_speed, 0, false );
              break;
            case ::SDLK_UP:
              viewport::pan( -movement_speed, 0, false );
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            viewport::scale_zoom( 0.98 );
          if( event.wheel.y > 0 )
            viewport::scale_zoom( 1.02 );
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
