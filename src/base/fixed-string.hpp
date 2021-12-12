/****************************************************************
**fixed-string.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-04.
*
* Description: Compile-time wrapper for strings allowing them to
*              be used as template arguments.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <array>
#include <string_view>

namespace base {

/****************************************************************
** fixed_string (Compile-Time String)
*****************************************************************/
// This is a string wrapper that can be used as a template para-
// meter. Be careful, this should probably only be used sparingly
// because when used as a template parameter it will cause the
// entire string to become part of the mangled name.
//
// Example:
//
//   template<base::fixed_string Arr>
//   struct parametrized_by_string {
//     ...
//     static constexpr std::string_view sv = Arr;
//     ...
//   };
//
template<int N>
/* clang-format off */
requires( N >= 0 )
struct fixed_string {
  /* clang-format on */
  static constexpr size_t kArrayLength  = N;
  static constexpr size_t kStringLength = N - 1;

  constexpr fixed_string( char const ( &arr )[N] ) {
    for( size_t i = 0; i < kStringLength; ++i ) data[i] = arr[i];
  }

  static constexpr size_t size() { return kStringLength; }

  static constexpr int ssize() {
    return static_cast<int>( kStringLength );
  }

  constexpr operator std::string_view() const {
    return std::string_view( data.data(), kStringLength );
  }

  auto operator<=>( fixed_string const& ) const = default;

  std::array<char, kStringLength> data = {};
};

} // namespace base
