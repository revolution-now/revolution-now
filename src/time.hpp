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
#include <cmath>
#include <thread>

namespace rn {

using Clock_t    = ::std::chrono::system_clock;
using Time_t     = decltype( Clock_t::now() );
using Duration_t = ::std::chrono::nanoseconds;
using Seconds    = ::std::chrono::seconds;

namespace chrono_literals = ::std::literals::chrono_literals;

template<class Rep, class Period>
void sleep(
    std::chrono::duration<Rep, Period> const& duration ) {
  std::this_thread::sleep_for( duration );
}

// Convert to the given chrono type. Will only work with `secs`
// within +-292 years.  Example:
//
//   using namespace std::chrono;
//   auto t = from_seconds<microseconds>( 1234.567 );
//
// and `t` will be of type microseconds.
template<typename ChronoDurationT, typename T>
auto from_seconds( T secs ) {
  auto nanos = std::lround( 1000000000.0 * secs );
  return std::chrono::duration_cast<ChronoDurationT>(
      std::chrono::nanoseconds( nanos ) );
}

} // namespace rn
