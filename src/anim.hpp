/****************************************************************
**anim.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-13.
*
* Description: Animation-related helpers.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "waitable.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <chrono>

namespace rn {

// Keep running func until it returns true. In between runs,
// pause for `pause` duration; if, after the pause, more time has
// actually passed (such as would happen if we have a frame rate
// that is too low to permit waiting the small amount of time
// that we'd like to) then the func will be run multiple times,
// to cover the number of times corresponding to the actual
// amount of time passed. In short, this function will allow run-
// ning animations that appear the same regardless of frame rate.
waitable<> animation_frame_throttler(
    std::chrono::microseconds pause, function_ref<bool()> func );

} // namespace rn
