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
#include "frame.hpp"

using namespace std;

namespace rn {

wait<> co_await_transform( FrameCount count ) {
  return wait_n_frames( count );
}

wait<> wait_n_frames( FrameCount n ) {
  if( n.frames == 0 ) return make_wait<>();
  wait_promise<> p;
  auto after_ticks = [p]() mutable { p.set_value_emplace(); };
  subscribe_to_frame_tick( after_ticks, n,
                           /*repeating=*/false );
  return p.wait();
}

} // namespace rn
