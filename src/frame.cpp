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
#include "math.hpp"
#include "plane.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

using namespace std;

namespace rn {

namespace {

MovingAverage<3 /*seconds*/> frame_rate;

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

uint64_t total_frame_count() { return frame_rate.total_ticks(); }
double   avg_frame_rate() { return frame_rate.average(); }

void frame_loop( bool poll_input, function<bool()> finished ) {
  using namespace chrono;

  auto frame_length = 1000ms / config_rn.target_frame_rate;

  while( true ) {
    frame_rate.tick();
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
      ::SDL_Delay(
          duration_cast<milliseconds>( frame_length - delta )
              .count() );
    }
  }
}

} // namespace rn
