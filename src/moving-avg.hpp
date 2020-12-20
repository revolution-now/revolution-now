/****************************************************************
**moving-avg.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-19.
*
* Description: Class that calculates moving averages over a
*              window.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "time.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <chrono>
#include <vector>

namespace rn {

// MovingAverage is a class for computing a moving average of a
// tick count over time. Conceptually, we have a time window of
// length `window_size` that slides and collects the tick count
// within its interval and maintains a (moving) average that can
// be retrieved at any time.
//
// The implementation, though, does not actually have a single
// window; it has multiple fixed buckets, and as the time goes
// on, new buckets come into activity while old ones get dropped.
// On each tick, all the active buckets will get their count in-
// creased. A bucket is active if its time window overlaps with
// "now". This allows the moving average to slide smoothly even
// if the window_size is large.
class MovingAverage {
  NON_COPYABLE( MovingAverage );

public:
  using QueryTimeFunc = std::chrono::system_clock::time_point();

  MovingAverage( std::chrono::seconds window_size );
  MovingAverage( std::chrono::seconds        window_size,
                 function_ref<QueryTimeFunc> now );

  // Add a count (to the count that is being time-averaged).
  void tick( int ticks = 1 );

  // This is used to update the state to account for the passage
  // of time even when no ticks are happening. This should not
  // interfere with any other calls to tick().
  void update();

  // Return the current average; this will be the average as com-
  // puted approximately (window_length) time ago.
  double average() const { return average_; }

  uint64_t total_ticks() const { return ticks_; }

private:
  using nanoseconds = std::chrono::nanoseconds;
  using seconds     = std::chrono::seconds;

  // Bumps the window one position forward; this will bring one
  // new bucket into the window and will remove one bucket off of
  // the end.
  void bump_slide();

  void advance_clock( nanoseconds ns );

  seconds const     kWindowSize;
  nanoseconds const kBucketSize;

  std::vector<uint64_t> buckets_;
  uint64_t              ticks_     = 0;
  Time_t                last_tick_ = {};
  nanoseconds           clock_{ 0 };
  double                average_ = 0.0;
  function_ref<std::chrono::system_clock::time_point()> now_;

  // This is an index into bucket_ that marks the start of the
  // window. The window starts there and goes til the end of
  // buckets_ and then wraps around, finishing at the element be-
  // fore curr_window_. This allows us to simulate a window
  // sliding over an infinite array of buckets using only a fi-
  // nite number of actual buckets.
  int curr_window_ = 0;
};

// FIXME: once clang gets support for the C++20 feature allowing
// simple class types to be used as template parameters, change
// the `int Seconds` to std::chrono::seconds.
template<int Seconds>
struct MovingAverageTyped : public MovingAverage {
  MovingAverageTyped()
    : MovingAverage( std::chrono::seconds( Seconds ) ) {}
  MovingAverageTyped( function_ref<QueryTimeFunc> now )
    : MovingAverage( std::chrono::seconds( Seconds ), now ) {}
};

} // namespace rn
