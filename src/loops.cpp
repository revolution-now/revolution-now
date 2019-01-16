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
#include "globals.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "physics.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "sdl-util.hpp"
#include "travel.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"

// base-util
#include "base-util/variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn {

namespace {} // namespace

void advance_viewport_translation() {
  auto const* __state = ::SDL_GetKeyboardState( nullptr );

  // Returns true if key is pressed.
  auto state = [__state]( ::SDL_Scancode code ) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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

    if( state( ::SDL_SCANCODE_A ) || state( ::SDL_SCANCODE_D ) ||
        state( ::SDL_SCANCODE_W ) || state( ::SDL_SCANCODE_S ) )
      viewport().stop_auto_panning();
  }
}

void take_input() {
  while( auto event = input::poll_event() )
    send_input_to_planes( *event );
}

void frame_throttler( bool             poll_input,
                      function<bool()> finished ) {
  using namespace std::chrono;
  using namespace std::literals::chrono_literals;

  auto frame_length = 1000ms / config_rn.target_frame_rate;
  // logger->debug( "frame length: {}ms", frame_length.count() );

  uint64_t frames = 0;
  // auto     initial = system_clock::now();

  while( true ) {
    ++frames;
    auto start = system_clock::now();

    draw_all_planes();
    ::SDL_RenderPresent( g_renderer );

    if( poll_input ) take_input();

    // TODO: put this in a method of Plane
    advance_viewport_translation();
    viewport().advance();

    if( finished() ) break;

    auto delta = system_clock::now() - start;
    if( delta < frame_length )
      ::SDL_Delay(
          duration_cast<milliseconds>( frame_length - delta )
              .count() );
  }

  // auto final       = system_clock::now();
  // auto average_fps = 1s / ( ( final - initial ) / frames );
  // logger->debug( "fps achieved: {}/s", average_fps );
}

} // namespace rn
