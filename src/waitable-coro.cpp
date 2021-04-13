/****************************************************************
**waitable-coro.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-31.
*
* Description: Enable waitable to work with coroutines.
*
*****************************************************************/
#include "waitable-coro.hpp"

// Revolution Now
#include "frame.hpp"

using namespace std;

namespace rn {

namespace detail {

waitable<> await_transform_impl( FrameCount frame_count ) {
  return wait_n_frames( frame_count );
}

waitable<std::chrono::microseconds> await_transform_impl(
    std::chrono::microseconds us ) {
  return wait_for_duration( us );
}

} // namespace detail

} // namespace rn
