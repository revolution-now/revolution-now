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
** Public API
*****************************************************************/
waitable<> animation_frame_throttler(
    chrono::microseconds pause, function_ref<bool()> func ) {
  using chrono::microseconds;
  microseconds accum{ 0 };
  while( true ) {
    accum += co_await pause;
    int animation_frames = accum / pause;
    accum                = accum - animation_frames * pause;
    for( int i = 0; i < animation_frames; ++i )
      if( func() ) co_return;
  }
}

} // namespace rn
