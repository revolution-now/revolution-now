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

// Revolution Now
#include "fmt-helper.hpp"

// C++ standard library
#include <chrono>
#include <cmath>
#include <thread>

namespace rn {

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

template<class Rep, class Period>
auto to_string_colons(
    std::chrono::duration<Rep, Period> const& duration ) {
  using namespace std::chrono;
  using namespace std::literals::chrono_literals;
  std::string res; // should use small-string optimization.
  auto        d = duration;
  if( d > 1h ) {
    auto hrs = duration_cast<hours>( d );
    res += fmt::format( "{:0>2}", hrs.count() );
    res += ':';
    d -= hrs;
  }
  if( d > 1min ) {
    auto mins = duration_cast<minutes>( d );
    res += fmt::format( "{:0>2}", mins.count() );
    res += ':';
    d -= mins;
  }
  auto secs = duration_cast<seconds>( d );
  res += fmt::format( "{:0>2}", secs.count() );
  return res;
}

} // namespace rn
