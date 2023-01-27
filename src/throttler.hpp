/****************************************************************
**throttler.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-13.
*
* Description: Throttler for animation timing.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <chrono>

namespace rn {

inline constexpr auto kFrameDuration =
    std::chrono::duration<int, std::ratio<1, 60>>{ 1 };

// This is a standard (1/60 s) frame, with a bit subtracted so
// that when we delay for one standard frame (in an animation)
// and we're running at 60Hz then we won't skip frames. This
// should probably be moved into a configuration file.
inline constexpr auto kAlmostStandardFrame =
    std::chrono::duration_cast<std::chrono::microseconds>(
        kFrameDuration ) -
    std::chrono::microseconds{ 300 };

// Animation frame rate throttler.  Example usage:
//
//   AnimThrottler throttle( kAlmostStandardFrame );
//   while( !terminate-condition ) {
//     co_await throttle();
//     ...
//   }
//
// The first frame will always run immediately, unless
// `initial_delay` is true. Also, the throttling line should be
// placed at the top of the loop so that any `continue` state-
// ments inside the loop don't have to have them.
struct AnimThrottler {
  using microseconds = std::chrono::microseconds;
  microseconds const gap;
  microseconds       accum;

  explicit AnimThrottler( microseconds gap_,
                          bool         initial_delay = false )
    : gap( gap_ ),
      accum( initial_delay ? microseconds{} : gap_ ) {}

  wait<> operator()();
};

} // namespace rn
