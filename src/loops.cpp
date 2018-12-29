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
#include "config-files.hpp"
#include "logging.hpp"
#include "movement.hpp"
#include "ownership.hpp"
#include "physics.hpp"
#include "render.hpp"
#include "sdl-util.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"

// base-util
#include "base-util/variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn {

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

void frame_throttler( function<bool()> frame ) {
  using namespace std::chrono;
  using namespace std::literals::chrono_literals;

  auto frame_length = 1000ms / config_rn.target_frame_rate;
  logger->debug( "frame length: {}ms", frame_length.count() );

  uint64_t frames  = 0;
  auto     initial = system_clock::now();

  while( true ) {
    ++frames;
    auto start = system_clock::now();

    render_frame();

    if( frame() ) break;

    auto delta = system_clock::now() - start;
    if( delta < frame_length )
      ::SDL_Delay(
          duration_cast<milliseconds>( frame_length - delta )
              .count() );
  }

  auto final       = system_clock::now();
  auto average_fps = 1s / ( ( final - initial ) / frames );
  logger->debug( "fps achieved: {}/s", average_fps );
}

orders_loop_result loop_orders() {
  orders_loop_result result{};
  result.type = orders_loop_result::e_type::none;

  e_push_direction zoom_direction = e_push_direction::none;

  ::SDL_Event event;
  // TODO: use the API in input.hpp
  while( SDL_PollEvent( &event ) != 0 ) {
    switch( event.type ) {
      case SDL_QUIT:
        result.type = orders_loop_result::e_type::quit_game;
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
          case ::SDLK_q:
            result.type = orders_loop_result::e_type::quit_game;
            break;
          case ::SDLK_F11: toggle_fullscreen(); break;
          case ::SDLK_t:
            result.type =
                orders_loop_result::e_type::orders_received;
            result.orders = orders::wait;
            break;
          case ::SDLK_SPACE:
          case ::SDLK_KP_5:
            result.type =
                orders_loop_result::e_type::orders_received;
            result.orders = orders::forfeight;
            break;
          default:
            auto maybe_direction =
                val_safe( nav_keys, event.key.keysym.sym );
            if( maybe_direction ) {
              result.type =
                  orders_loop_result::e_type::orders_received;
              result.orders = orders::move{*maybe_direction};
              break;
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
    if( result.type != orders_loop_result::e_type::none )
      // Break as soon as we have some orders.
      break;
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
  return result;
}

e_eot_loop_result loop_eot() {
  e_eot_loop_result result         = e_eot_loop_result::none;
  e_push_direction  zoom_direction = e_push_direction::none;

  ::SDL_Event event;
  while( SDL_PollEvent( &event ) != 0 ) {
    switch( event.type ) {
      case SDL_QUIT:
        result = e_eot_loop_result::quit_game;
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
            result = e_eot_loop_result::quit_game;
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
  return result;
}

bool loop_mv_unit( double&              percent,
                   DissipativeVelocity& percent_vel ) {
  percent_vel.advance( e_push_direction::none );
  percent += percent_vel.to_double();
  return ( percent > 1.0 );
}

} // namespace rn
