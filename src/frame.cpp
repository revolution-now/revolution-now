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
#include "ranges.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

using namespace std;
using namespace literals::chrono_literals;

namespace rn {

namespace {

constexpr auto window_length  = 3s;
constexpr auto window_spacing = 100ms;
constexpr auto num_active_windows =
    window_length / window_spacing;

static_assert( window_length > window_spacing );
static_assert( window_length % window_spacing == 0s );
static_assert( num_active_windows > 1 );

uint64_t         g_total_frames{0};
vector<uint64_t> g_frame_count_windows( num_active_windows, 0 );
uint64_t         g_frames_in_last_window{0};
chrono::nanoseconds g_slide_time_accum{0};

auto sliding_window = g_frame_count_windows //
                      | rv::cycle           //
                      | rv::sliding( num_active_windows );

auto curr_slide = sliding_window.begin();

void bump_slide() {
  g_frames_in_last_window = *( ( *curr_slide ).begin() );
  ++curr_slide;
  auto& last =
      *( ( *curr_slide ) | rv::reverse | rv::take( 1 ) ).begin();
  last = 0;
}

void add_frame_tick() {
  for( auto& window : *curr_slide ) ++window;
  g_total_frames++;
}

void add_frame_time( chrono::nanoseconds ns ) {
  g_slide_time_accum += ns;
  auto bumps         = g_slide_time_accum / window_spacing;
  g_slide_time_accum = g_slide_time_accum % window_spacing;
  for( ; bumps > 0; --bumps ) bump_slide();
}

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

uint64_t total_frame_count() { return g_total_frames; }

double avg_frame_rate() {
  auto average_fps =
      g_frames_in_last_window /
      double(
          chrono::duration_cast<chrono::seconds>( window_length )
              .count() );
  return average_fps;
}

void frame_loop( bool poll_input, function<bool()> finished ) {
  using namespace chrono;

  auto frame_length = 1000ms / config_rn.target_frame_rate;

  while( true ) {
    add_frame_tick();
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
    auto finish = system_clock::now() - start;
    add_frame_time( finish );
  }
}

} // namespace rn
