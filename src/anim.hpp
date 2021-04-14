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

// Animation frame rate throttler.  Example usage:
//
//   AnimThrottler throttle( kAlmostStandardFrame );
//   while( !terminate-condition ) {
//     co_await throttle();
//     ...
//   }
//
// The first frame will always run immediately. Also, the throt-
// tling line should be placed at the top of the loop so that any
// `continue` statements inside the loop don't have to have them.
struct AnimThrottler {
  using microseconds = std::chrono::microseconds;
  microseconds const gap;
  microseconds       accum;

  explicit AnimThrottler( microseconds gap_ )
    : gap( gap_ ), accum( gap_ ) {}

  waitable<> operator()();
};

} // namespace rn
