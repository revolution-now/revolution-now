/****************************************************************
**time.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-17.
*
* Description: All things in the fourth dimension.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <chrono>
#include <thread>

namespace rn {

template<class Rep, class Period>
void sleep(
    std::chrono::duration<Rep, Period> const& duration ) {
  std::this_thread::sleep_for( duration );
}

} // namespace rn
