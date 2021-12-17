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

} // namespace rn
