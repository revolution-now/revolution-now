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

waitable<std::chrono::milliseconds> await_transform_impl(
    std::chrono::milliseconds ms ) {
  return wait_for_duration( ms );
}

} // namespace detail

} // namespace rn
