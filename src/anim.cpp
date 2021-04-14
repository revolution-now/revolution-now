/****************************************************************
**anim.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-13.
*
* Description: Animation-related helpers.
*
*****************************************************************/
#include "anim.hpp"

// Revolution Now
#include "frame.hpp"
#include "waitable-coro.hpp"

using namespace std;

namespace rn {

/****************************************************************
** AnimThrottler
*****************************************************************/
waitable<> AnimThrottler::operator()() {
  // Need >= so that the first frame runs immediately.
  if( accum >= gap ) {
    accum -= gap;
    co_return;
  }
  accum += co_await( gap - accum );
  accum -= gap;
}

/****************************************************************
** Public API
*****************************************************************/
waitable<> animation_frame_throttler(
    chrono::microseconds pause, function_ref<bool()> func ) {
  AnimThrottler throttle( pause );
  do { co_await throttle(); } while( func() );
}

} // namespace rn
