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
#include "compositor.hpp" // FIXME: temporary
#include "config-files.hpp"
#include "input.hpp"
#include "macros.hpp"
#include "math.hpp"
#include "plane.hpp"
#include "render.hpp" // FIXME
#include "screen.hpp"
#include "variant.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

// base
#include "base/scope-exit.hpp"
#include "base/variant.hpp"

// Range-v3
#include "range/v3/algorithm/any_of.hpp"

// C++ standard library
#include <thread>

using namespace std;

namespace rn {

namespace {

MovingAverage<3 /*seconds*/> frame_rate;

EventCountMap g_event_counts;

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
    base::variant<FrameSubscriptionTick, FrameSubscriptionTime>;
NOTHROW_MOVE( FrameSubscription );

vector<FrameSubscription>& subscriptions() {
  static vector<FrameSubscription> subs;
  return subs;
}

void notify_subscribers() {
  for( auto& sub : subscriptions() ) {
    overload_visit(
        sub,
        []( FrameSubscriptionTick& tick_sub ) {
          auto& [interval, last_message, func] = tick_sub;
          auto total = total_frame_count();
          if( long( total ) - last_message > interval ) {
            last_message = total;
            func();
          }
        },
        []( FrameSubscriptionTime& time_sub ) {
          auto& [interval, last_message, func] = time_sub;
          auto now                             = Clock_t::now();
          if( now - last_message > interval ) {
            last_message = now;
            func();
          }
        } );
  }
}

using InputReceivedFunc = tl::function_ref<void()>;
using FrameLoopBodyFunc =
    tl::function_ref<bool( InputReceivedFunc )>;

void frame_loop_impl( FrameLoopBodyFunc body ) {
  // FIXME: temporary.
  static bool guard = false;
  CHECK( !guard, "cannot re-enter frame_loop function." );
  guard = true;
  SCOPE_EXIT( guard = false );

  using namespace chrono;

  auto normal_frame_length =
      1000000us / config_rn.target_frame_rate;
  auto slow_frame_length = 1000000us / 5;

  static auto time_of_last_input = Clock_t::now();

  while( true ) {
    // If we go more than the configured time without any user
    // input then slow down the frame rate to save battery.
    auto frame_length = ( Clock_t::now() - time_of_last_input >
                          config_rn.power.time_till_slow_fps )
                            ? slow_frame_length
                            : normal_frame_length;

    auto start = system_clock::now();
    frame_rate.tick();
    auto on_input = [] { time_of_last_input = Clock_t::now(); };
    // ----------------------------------------------------------
    if( body( on_input ) ) return;
    // ----------------------------------------------------------
    auto delta = system_clock::now() - start;
    if( delta < frame_length )
      this_thread::sleep_for( frame_length - delta );
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

bool advance_all_state() {
  if( advance_app_state() ) return true;
  advance_plane_state();
  return false;
}

void frame_loop() {
  frame_loop_impl( []( InputReceivedFunc input_received ) {
    // ----------------------------------------------------------
    // 1. Get Input.
    input::pump_event_queue();

    auto is_win_resize = []( auto const& e ) {
      if_get( e, input::win_event_t, val ) {
        return val.type == input::e_win_event_type::resized;
      }
      return false;
    };

    auto& q = input::event_queue();
    while( q.size() > 0 ) {
      input_received();
      ASSIGN_CHECK_OPT( event, q.front() );
      if( is_win_resize( event ) ) on_main_window_resized();
      (void)send_input_to_planes( event );
      q.pop();
    }

    // ----------------------------------------------------------
    // 2. Update State.
    if( advance_all_state() ) return true;

    // This invokes (synchronous/blocking) callbacks to any sub-
    // scribers that want to be notified at regular tick or time
    // intervals.
    notify_subscribers();

    // Keep the state of the moving averages up to date even when
    // there are no ticks happening on them. Specifically, if
    // there are no ticks happening, then this will slowly cause
    // the average to drop.
    for( auto& p : g_event_counts ) p.second.update();

    // ----------------------------------------------------------
    // 3. Draw.
    draw_all_planes();
    ::SDL_RenderPresent( g_renderer );

    return false;
  } );
}

} // namespace rn
