/****************************************************************
**throttler.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-13.
*
* Description: Throttler for animation timing.
*
*****************************************************************/
#include "throttler.hpp"

// Revolution Now
#include "co-time.hpp"
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** AnimThrottler
*****************************************************************/
wait<> AnimThrottler::operator()() {
  // Need >= so that the first frame runs immediately.
  if( accum >= gap ) {
    accum -= gap;
    co_return;
  }
  accum += co_await ( gap - accum );
  accum -= gap;
}

} // namespace rn
