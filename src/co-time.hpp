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

// When co_await'ing on a duration, the coroutine will be sus-
// pended for at least that much time. The result upon resumption
// will be actual amount of time suspended.
wait<std::chrono::microseconds> co_await_transform(
    std::chrono::microseconds us );

} // namespace rn
