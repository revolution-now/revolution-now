/****************************************************************
**frame.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-13.
*
* Description: Frames and frame rate.
*
*****************************************************************/
#include "frame.hpp"

// Revolution Now
#include "config-files.hpp"
#include "globals.hpp"
#include "input.hpp"
#include "plane.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

using namespace std;

namespace rn {

namespace {

uint64_t            g_frames = 0;
chrono::nanoseconds g_frame_time{0};

void take_input() {
  while( auto event = input::poll_event() )
    send_input_to_planes( *event );
}

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

} // namespace

double avg_frame_rate() {
  using namespace literals::chrono_literals;
  auto average_fps = 1s / ( g_frame_time / g_frames );
  return average_fps;
}

void frame_loop( bool poll_input, function<bool()> finished ) {
  using namespace chrono;
  using namespace literals::chrono_literals;

  auto frame_length = 1000ms / config_rn.target_frame_rate;

  auto initial = system_clock::now();

  while( true ) {
    ++g_frames;
    auto start = system_clock::now();

    draw_all_planes();
    ::SDL_RenderPresent( g_renderer );

    if( poll_input ) take_input();

    // TODO: put this in a method of Plane
    advance_viewport_translation();
    viewport().advance();

    if( finished() ) break;

    auto delta = system_clock::now() - start;
    if( delta < frame_length ) {
      // TODO: are these casts necessary?
      ::SDL_Delay(
          duration_cast<milliseconds>( frame_length - delta )
              .count() );
    }
  }
  g_frame_time += system_clock::now() - initial;
}

} // namespace rn
