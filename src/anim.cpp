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
#include "co-time.hpp"
#include "co-waitable.hpp"

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

} // namespace rn
