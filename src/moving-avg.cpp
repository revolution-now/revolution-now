/****************************************************************
**moving-avg.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-19.
*
* Description: Class that calculates moving averages over a
*              window.
*
*****************************************************************/
#include "moving-avg.hpp"

// Revolution Now
#include "error.hpp"

// base
#include "base/math.hpp"

namespace rn {

using namespace std;
using std::chrono::seconds;

constexpr int kNumBuckets = 25;

MovingAverage::MovingAverage(
    seconds window_size, base::function_ref<QueryTimeFunc> now )
  : kWindowSize( window_size ),
    kBucketSize( nanoseconds{ window_size } / kNumBuckets ),
    buckets_( kNumBuckets ),
    now_( now ) {
  CHECK( kWindowSize % kBucketSize == nanoseconds{ 0 } );
  CHECK( kNumBuckets > 1 );
  fill( buckets_.begin(), buckets_.end(), 0 );
}

MovingAverage::MovingAverage( seconds window_size )
  : MovingAverage( window_size, chrono::system_clock::now ) {}

void MovingAverage::tick( int ticks ) {
  update();
  for( auto& bucket : buckets_ ) bucket += ticks;
  ticks_ += ticks;
}

void MovingAverage::update() {
  auto now = now_();
  if( last_tick_ != Time_t{} || ticks_ > 0 )
    advance_clock( now - last_tick_ );
  last_tick_ = now;
}

void MovingAverage::bump_slide() {
  auto ticks_in_last_bucket_ = buckets_[curr_window_];
  average_ =
      ticks_in_last_bucket_ / double( kWindowSize.count() );
  // Now slide the window forward and initialize the new bucket
  // to zero. It's not really a new bucket since we reuse ex-
  // isting buckets, but conceptually it's a new bucket since
  // we're conceptually sliding the window over an infinite array
  // of buckets, just that we're reusing old buckets so that we
  // don't need an infinite amount of memory.
  curr_window_ = ( curr_window_ + 1 ) % buckets_.size();
  // This  is the first bucket of the new window, a.k.a. the one
  // we need to initialize to zero since it (conceptually) does
  // not yet have any ticks.
  auto& last = buckets_[base::cyclic_modulus(
      curr_window_ - 1, int( buckets_.size() ) )];
  last       = 0;
}

void MovingAverage::advance_clock( nanoseconds ns ) {
  clock_ += ns;
  auto bumps = clock_ / kBucketSize;
  clock_     = clock_ % kBucketSize;
  // This loop should only run once (i.e., bumps should == 1)
  // if this `advance_clock` function is called often (as it
  // should be). However, if for some reason the time since the
  // last call is more than a kBucketSize then we will bump
  // it twice (note that, although that is the correct thing to
  // do, the average computed from that MovingAverage object
  // will not be very reliable).
  for( ; bumps > 0; --bumps ) bump_slide();
}

} // namespace rn
