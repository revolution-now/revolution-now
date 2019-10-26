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
#include "app-state.hpp"
#include "config-files.hpp"
#include "input.hpp"
#include "macros.hpp"
#include "math.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "variant.hpp"
#include "viewport.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

// C++ standard library
#include <thread>

using namespace std;

namespace rn {

namespace {

MovingAverage<3 /*seconds*/> frame_rate;

EventCountMap g_event_counts;

// Returns true if any input was received.
ND bool take_input() {
  bool received_input = false;
  while( auto event = input::next_event() ) {
    // Just in case we have resized the main window we need to
    // call this before dispatching it to the planes.
    viewport().enforce_invariants(); // FIXME: this is icky here.
    send_input_to_planes( *event );
    received_input = true;
  }
  return received_input;
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

struct FrameSubscriptionTick {
  int                   interval{ 1 };
  int                   last_message{ 0 };
  FrameSubscriptionFunc func;
};
NOTHROW_MOVE( FrameSubscriptionTick );

struct FrameSubscriptionTime {
  chrono::milliseconds  interval;
  Time_t                last_message{};
  FrameSubscriptionFunc func;
};
NOTHROW_MOVE( FrameSubscriptionTime );

using FrameSubscription =
    variant<FrameSubscriptionTick, FrameSubscriptionTime>;
NOTHROW_MOVE( FrameSubscription );

vector<FrameSubscription>& subscriptions() {
  static vector<FrameSubscription> subs;
  return subs;
}

void notify_subscribers() {
  for( auto& sub : subscriptions() ) {
    switch_( sub ) {
      case_( FrameSubscriptionTick, interval, last_message,
             func ) {
        auto total = total_frame_count();
        if( long( total ) - last_message > interval ) {
          last_message = total;
          func();
        }
      }
      case_( FrameSubscriptionTime, interval, last_message,
             func ) {
        auto now = Clock_t::now();
        if( now - last_message > interval ) {
          last_message = now;
          func();
        }
      }
      switch_exhaustive;
    }
  }
}

} // namespace

void subscribe_to_frame_tick( FrameSubscriptionFunc func,
                              int                   n ) {
  subscriptions().push_back( FrameSubscriptionTick{
      /*interval=*/n, /*last_message=*/0, /*func=*/func } );
}

void subscribe_to_frame_tick( FrameSubscriptionFunc     func,
                              std::chrono::milliseconds n ) {
  subscriptions().push_back( FrameSubscriptionTime{
      /*interval=*/n, /*last_message=*/Clock_t::now(),
      /*func=*/func } );
}

EventCountMap& event_counts() { return g_event_counts; }

uint64_t total_frame_count() { return frame_rate.total_ticks(); }
double   avg_frame_rate() { return frame_rate.average(); }

void frame_loop( bool                     poll_input,
                 tl::function_ref<bool()> finished ) {
  using namespace chrono;

  auto normal_frame_length =
      1000000us / config_rn.target_frame_rate;
  auto slow_frame_length = 1000000us / 5;

  static auto time_of_last_input = Clock_t::now();

  while( true ) {
    advance_app_state();
    // Must call this at the start of the frame before doing any-
    // thing else. This calls an update method on each plane to
    // allow it to update any internal state that it has each
    // frame that must be done at start of frame.
    update_all_planes_start();

    // If we go more than the configured time without any user
    // input then slow down the frame rate to save battery.
    auto frame_length = ( Clock_t::now() - time_of_last_input >
                          config_rn.power.time_till_slow_fps )
                            ? slow_frame_length
                            : normal_frame_length;

    auto start = system_clock::now();
    frame_rate.tick();
    // Keep the state of the moving averages up to date even when
    // there are no ticks happening on them. Specifically, if
    // there are no ticks happening, then this will slowly cause
    // the average to drop.
    for( auto& p : g_event_counts ) p.second.update();

    draw_all_planes();
    ::SDL_RenderPresent( g_renderer );

    if( poll_input )
      if( take_input() ) //
        time_of_last_input = Clock_t::now();

    // TODO: put this in a method of Plane
    advance_viewport_translation();
    viewport().advance();

    // This invokes (synchronous/blocking) callbacks to any sub-
    // scribers that want to be notified at regular tick or time
    // intervals.
    notify_subscribers();

    // Must call this at the end of the frame after doing every-
    // thing else. This calls an update method on each plane to
    // allow it to update any internal state that it has each
    // frame that must be done at end of frame.
    update_all_planes_end();

    if( finished() ) break;

    auto delta = system_clock::now() - start;
    if( delta < frame_length )
      this_thread::sleep_for( frame_length - delta );
  }
}

} // namespace rn
