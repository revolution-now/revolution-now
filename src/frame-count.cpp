/****************************************************************
**frame-count.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-21.
*
* Description: Defines a strong int representing a frame count.
*
*****************************************************************/
#include "frame-count.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "frame.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

wait<> co_await_transform( FrameCount count ) {
  return wait_n_frames( count );
}

wait<> wait_n_frames( FrameCount n ) {
  if( n.frames == 0 ) co_return;
  wait_promise<> p;
  auto after_ticks = [&p]() mutable { p.set_value_emplace(); };
  int64_t const subscription_id =
      subscribe_to_frame_tick( after_ticks, n,
                               /*repeating=*/false );
  SCOPE_EXIT( unsubscribe_frame_tick( subscription_id ) );
  // Need to keep p alive.
  co_await p.wait();
}

} // namespace rn
