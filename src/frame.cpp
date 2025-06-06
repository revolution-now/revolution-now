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
#include "co-runner.hpp"
#include "iengine.hpp"
#include "input.hpp"
#include "macros.hpp"
#include "moving-avg.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "time.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/logical.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// base
#include "base/function-ref.hpp"
#include "base/lambda.hpp"
#include "base/variant-util.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <thread>

using namespace std;

namespace rn {

namespace {

int g_target_fps = 60;

MovingAverage frame_rate( chrono::seconds( 3 ) );

EventCountMap g_event_counts;

struct FrameSubscriptionTick {
  bool done = false; // for one-time notifications
  FrameCount interval{ 1 };
  uint64_t last_message{ 0 };
  FrameSubscriptionFunc func;
};

struct FrameSubscriptionTime {
  bool done = false; // for one-time notifications
  chrono::microseconds interval;
  Time_t last_message{};
  FrameSubscriptionFunc func;
};

using FrameSubscriptionData =
    base::variant<FrameSubscriptionTick, FrameSubscriptionTime>;

struct FrameSubscription {
  inline static int64_t next_id = 1;

  FrameSubscription( FrameSubscriptionData&& data )
    : data( std::move( data ) ) {
    id = next_id++;
  }

  FrameSubscriptionData data;
  int64_t id = 0;
};

vector<FrameSubscription>& subscriptions() {
  static vector<FrameSubscription> subs;
  return subs;
}

vector<FrameSubscription>& subscriptions_oneoff() {
  static vector<FrameSubscription> subs;
  return subs;
}

void notify_subscribers( bool const force = false ) {
  auto try_notify = [&]( FrameSubscription& sub ) {
    base::overload_visit(
        sub.data,
        [&]( FrameSubscriptionTick& tick_sub ) {
          auto& [done, interval, last_message, func] = tick_sub;
          auto total = total_frame_count();
          if( force ||
              int( total - last_message ) >= interval.frames ) {
            last_message = total;
            func();
            done = true;
          }
        },
        [&]( FrameSubscriptionTime& time_sub ) {
          auto& [done, interval, last_message, func] = time_sub;
          auto now = Clock_t::now();
          if( force || now - last_message >= interval ) {
            last_message = now;
            func();
            done = true;
          }
        } );
  };
  for( auto& sub : subscriptions() ) try_notify( sub );
  for( auto& sub : subscriptions_oneoff() ) try_notify( sub );
  erase_if( subscriptions_oneoff(), []( FrameSubscription& fs ) {
    return std::visit( L( _.done ), fs.data );
  } );
}

struct DeferredEvents {
  vector<input::resolution_event_t> resolution;
};

using InputReceivedFunc = base::function_ref<void()>;
using FrameLoopBodyFunc = base::function_ref<void(
    IEngine&, Planes&, DeferredEvents&, InputReceivedFunc )>;

void frame_loop_scheduler( IEngine& engine, wait<> const& what,
                           Planes& planes,
                           FrameLoopBodyFunc body ) {
  using namespace chrono;

  constexpr auto slow_frame_length = 1000000us / 5;

  static auto time_of_last_input = Clock_t::now();

  DeferredEvents deferred_events;

  while( !what.ready() && !what.has_exception() ) {
    microseconds normal_frame_length = 1000000us / g_target_fps;
    // If we go more than the configured time without any user
    // input then slow down the frame rate to save battery.
    auto frame_length = ( Clock_t::now() - time_of_last_input >
                          config_rn.power.time_till_slow_fps )
                            ? slow_frame_length
                            : normal_frame_length;

    auto start    = system_clock::now();
    auto on_input = [] { time_of_last_input = Clock_t::now(); };
    // ----------------------------------------------------------
    body( engine, planes, deferred_events, on_input );
    // ----------------------------------------------------------
    auto delta = system_clock::now() - start;
    if( delta < frame_length )
      this_thread::sleep_for( frame_length - delta );
    // Should be done last otherwise if a coroutine that runs be-
    // fore the very first suspension of the overall coroutine
    // wants to skip a frame then it won't. I.e., we need to make
    // sure that the first frame body runs with a frame index
    // (tick) value of 0, not 1.
    frame_rate.tick();
  }

  if( what.has_exception() ) {
    base::ExceptionInfo const info =
        base::rethrow_and_get_info( what.exception() );
    FATAL( "uncaught exception of type `{}` in coroutine: {}",
           info.demangled_type_name, info.msg );
  }
}

static bool try_defer( DeferredEvents& deferred_events,
                       input::event_t const& e ) {
  using namespace input;
  SWITCH( e ) {
    CASE( resolution_event ) {
      deferred_events.resolution.push_back( resolution_event );
      return true;
    }
    default:
      return false;
  }
}

// Called once per frame.
void frame_loop_body( IEngine& engine, Planes& planes,
                      DeferredEvents& deferred_events,
                      InputReceivedFunc input_received ) {
  rr::Renderer& renderer =
      engine.renderer_use_only_when_needed();

  // ----------------------------------------------------------
  // Step: Notify

  // This invokes (synchronous/blocking) callbacks to any sub-
  // scribers that want to be notified at regular tick or time
  // intervals.
  notify_subscribers();
  run_all_coroutines();

  // Keep the state of the moving averages up to date even when
  // there are no ticks happening on them. Specifically, if there
  // are no ticks happening, then this will slowly cause the av-
  // erage to drop.
  for( auto& p : g_event_counts ) p.second.update();

  // ----------------------------------------------------------
  // Step: Process deferred resolution events.
  for( input::resolution_event_t const& event :
       deferred_events.resolution ) {
    on_logical_resolution_changed(
        engine.video(), engine.window(), renderer,
        engine.resolutions(), event.resolutions.get() );
    planes.get().input( event );
    run_all_coroutines();
    if( auto const rs =
            get_selected_resolution( event.resolutions );
        rs.has_value() ) {
      planes.on_logical_resolution_changed( rs->named );
      run_all_coroutines();
    }
  }
  deferred_events.resolution.clear();

  // ----------------------------------------------------------
  // Step: Get Input.
  input::pump_event_queue( engine );

  for( auto& q = input::event_queue(); !q.empty(); ) {
    input_received();
    input::event_t const& event = q.front();
    bool const was_deferred =
        try_defer( deferred_events, event );
    if( !was_deferred ) planes.get().input( event );
    q.pop();
    run_all_coroutines();
  }

  // ----------------------------------------------------------
  // Step: Update State.
  planes.get().advance_state();
  run_all_coroutines();

  // ----------------------------------------------------------
  // Step: Draw.
  auto const drawer = [&]( rr::Renderer& renderer ) {
    renderer.clear_screen( gfx::pixel::black() );
    planes.get().draw( renderer );
  };
  renderer.render_pass( drawer );
};

void deinit_frame() {
  subscriptions().clear();
  subscriptions_oneoff().clear();
}

} // namespace

void testing_notify_all_subscribers() {
  notify_subscribers( /*force=*/true );
}

int64_t subscribe_to_frame_tick( FrameSubscriptionFunc func,
                                 FrameCount n, bool repeating ) {
  auto& vec =
      ( repeating ? subscriptions : subscriptions_oneoff )();
  vec.push_back( FrameSubscription(
      FrameSubscriptionTick{ .done         = false,
                             .interval     = n,
                             .last_message = total_frame_count(),
                             .func         = func } ) );
  return vec.back().id;
}

int64_t subscribe_to_frame_tick( FrameSubscriptionFunc func,
                                 chrono::microseconds n,
                                 bool repeating ) {
  auto& vec =
      ( repeating ? subscriptions : subscriptions_oneoff )();
  vec.push_back( FrameSubscription(
      FrameSubscriptionTime{ .done         = false,
                             .interval     = n,
                             .last_message = Clock_t::now(),
                             .func         = func } ) );
  return vec.back().id;
}

void unsubscribe_frame_tick( int64_t id ) {
  erase_if( subscriptions(), [&]( FrameSubscription& fs ) {
    return fs.id == id;
  } );
  erase_if(
      subscriptions_oneoff(),
      [&]( FrameSubscription& fs ) { return fs.id == id; } );
}

EventCountMap& event_counts() { return g_event_counts; }

uint64_t total_frame_count() { return frame_rate.total_ticks(); }
double avg_frame_rate() { return frame_rate.average(); }

void frame_loop( IEngine& engine, Planes& planes,
                 wait<> const& what ) {
  g_target_fps = config_gfx.target_frame_rate;
  frame_loop_scheduler( engine, what, planes, frame_loop_body );
  deinit_frame();
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( get_target_framerate, int ) { return g_target_fps; }

LUA_FN( set_target_framerate, void, int target ) {
  CHECK( target > 0 );
  CHECK( target < 1000 );
  g_target_fps = target;
}

} // namespace

} // namespace rn
