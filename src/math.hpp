/****************************************************************
**math.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-17.
*
* Description: Various math and statistics utilities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "ranges-fwd.hpp"
#include "time.hpp"

// Range-v3 (FIXME: get this out of the interface)
#include "range/v3/view/cycle.hpp"
#include "range/v3/view/reverse.hpp"
#include "range/v3/view/sliding.hpp"

// C++ standard library
#include <array>
#include <chrono>
#include <type_traits>

namespace rn {

template<typename T,
         typename = std::enable_if_t<std::is_signed_v<T> &&
                                     std::is_integral_v<T>>>
T modulus( T a, T b ) {
  auto m = a % b;
  if( m < 0 ) m = ( b < 0 ) ? m - b : m + b;
  return m;
}

// MovingAverage is a class for computing a moving average of a
// tick count over time. Conceptually, we have a time window of
// length BucketLengthSeconds that slides and collects the tick
// count within its interval and maintains a (moving) average
// that can be retrieved at any time.
//
// The implementation, though, does not actually have a single
// window; it has multiple fixed buckets, and as the time goes
// on, new buckets come into activity while old ones get dropped.
// On each tick, all the active buckets will get their count in-
// creased. A bucket is active if its time window overlaps with
// "now".
//
// Ideally we'd be able to use nanoseconds here as the template
// parameter (FIXME: C++20 will allow this!)
//
// Note: these are non-copyable and non-movable to simplify
// implementation. If we were to move or copy them then we'd have
// to also reinitailize the range view members since they'd be
// referring to data members of the copied-from object.
template<int BucketLengthSeconds>
class MovingAverage {
  NO_COPY_NO_MOVE( MovingAverage );
  using nanoseconds = std::chrono::nanoseconds;
  using seconds     = std::chrono::seconds;

  constexpr static seconds bucket_length{ BucketLengthSeconds };
  constexpr static nanoseconds bucket_spacing =
      nanoseconds{ bucket_length } / 30;
  constexpr static auto num_buckets =
      bucket_length / bucket_spacing;

  static_assert( bucket_length > nanoseconds{ 30 } );
  static_assert( bucket_length % bucket_spacing ==
                 nanoseconds{ 0 } );
  static_assert( num_buckets > 1 );

public:
  MovingAverage() {
    for( auto& bucket : buckets_ ) bucket = 0;
  }

  void tick() {
    auto now = std::chrono::system_clock::now();
    if( ticks_ > 0 ) advance_clock( now - last_tick_ );
    last_tick_ = now;
    for( auto& bucket : *curr_window ) ++bucket;
    ticks_++;
  }

  // This is used to update the state to account for the passage
  // of time even when no ticks are happening. This should not
  // interfere with any other calls to tick().
  void update() {
    auto now = std::chrono::system_clock::now();
    if( last_tick_ != Time_t{} )
      advance_clock( now - last_tick_ );
    last_tick_ = now;
  }

  // Return the current average; this will be the average as com-
  // puted approximately (window_length) time ago.
  double average() const { return average_; }

  uint64_t total_ticks() const { return ticks_; }

private:
  // Bumps the window one position forward; this will bring one
  // new bucket into the window and will remove one bucket off of
  // the end.
  void bump_slide() {
    auto ticks_in_last_bucket_ = *( *curr_window ).begin();
    ++curr_window;
    // This  is the last bucket of the new window, a.k.a. the one
    // we need to initialize to zero since it (conceptually) does
    // not yet have any ticks.
    auto& last = *( ( *curr_window ) | rv::reverse ).begin();
    last       = 0;
    average_ =
        ticks_in_last_bucket_ / double( bucket_length.count() );
  }

  void advance_clock( nanoseconds ns ) {
    clock_ += ns;
    auto bumps = clock_ / bucket_spacing;
    clock_     = clock_ % bucket_spacing;
    // This loop should only run once (i.e., bumps should == 1)
    // if this `advance_clock` function is called often (as it
    // should be). However, if for some reason the time since the
    // last call is more than a bucket_spacing then we will bump
    // it twice (note that, although that is the correct thing to
    // do, the average computed from that MovingAverage object
    // will not be very reliable).
    for( ; bumps > 0; --bumps ) bump_slide();
  }

  using Buckets = std::array<uint64_t, num_buckets>;

  Buckets     buckets_;
  uint64_t    ticks_{ 0 };
  Time_t      last_tick_{};
  nanoseconds clock_{ 0 };
  double      average_{ 0.0 };

  // This window encompases multiple buckets.
  using SlideView   = decltype( buckets_    //
                              | rv::cycle //
                              | rv::sliding( num_buckets ) );
  using SlideViewIt = decltype( SlideView{}.begin() );

  SlideView sliding_window = buckets_    //
                             | rv::cycle //
                             | rv::sliding( num_buckets );

  SlideViewIt curr_window = sliding_window.begin();
};

} // namespace rn
