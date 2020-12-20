/****************************************************************
**math.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-17.
*
* Description: Various math and statistics utilities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <type_traits>

namespace rn {

template<typename T>
constexpr T modulus( T a, T b ) requires(
    std::is_signed_v<T>&& std::is_integral_v<T> ) {
  auto m = a % b;
  if( m < 0 ) m = ( b < 0 ) ? m - b : m + b;
  return m;
}

} // namespace rn
