/****************************************************************
**co-time.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-21.
*
* Description: Time-related wait and coroutine helpers.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// C++ standard library
#include <chrono>

namespace rn {

// This allows co_await'ing directly on a duration, e.g.:
//
//   co_await chrono::seconds( 1 );
//
// When co_await'ing on a duration, the coroutine will be sus-
// pended for at least that much time. The result upon resumption
// will be actual amount of time suspended.
wait<std::chrono::microseconds> co_await_transform(
    std::chrono::microseconds us );

// The returned wait becomes ready after the given duration
// has passed, and it returns the actual duration that has
// passed. This is useful if co_await'ing on small time intervals
// and the frame rate is low. Note: instead of co_await'ing this
// directly, you can do: co_await 2s.
wait<std::chrono::microseconds> wait_for_duration(
    std::chrono::microseconds us );

} // namespace rn

// Hack to allow finding this via ADL. If some day this conflicts
// with something in std, then just rename it.
namespace std {
using ::rn::co_await_transform;
}
