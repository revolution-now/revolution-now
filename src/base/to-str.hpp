/****************************************************************
**to-str.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-07.
*
* Description: to_str framework.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "adl-tag.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <concepts>
#include <string>
#include <string_view>

namespace base {

void to_str( int const& o, std::string& out, ADL_t );
void to_str( char o, std::string& out, ADL_t );
void to_str( double o, std::string& out, ADL_t );
void to_str( std::string_view o, std::string& out, ADL_t );

// Use a template for bool to prevent implicit conversions.
template<std::same_as<bool> B>
inline void to_str( B o, std::string& out, ADL_t ) {
  if( o )
    out += "true";
  else
    out += "false";
}

template<size_t N>
void to_str( char const ( &o )[N], std::string& out, ADL_t ) {
  to_str( std::string_view( o ), out, ADL );
}

template<size_t N>
void to_str( char ( &o )[N], std::string& out, ADL_t ) {
  to_str( std::string_view( o ), out, ADL );
}

template<typename T>
concept Show = requires( T o, std::string s ) {
  { to_str( o, s, ADL ) } -> std::same_as<void>;
};

// Only use this one when you know that you're only converting a
// single value to a string. Otherwise, prefer the to_str variant
// with an output argument because it reuses the same string ob-
// ject for efficiency.
template<Show T>
std::string to_str( T&& o ) {
  std::string res;
  to_str( o, res, ADL );
  return res;
}

} // namespace base

namespace fmt {

namespace detail {

// Need to exclude these because they would conflict with the
// ones in fmt.
template<typename T>
concept ExcludeFromFmtPromotion = requires {
  // clang-format off
  requires(
      std::is_same_v<T, std::string>             ||
     (std::is_scalar_v<T> && !std::is_enum_v<T>) ||
      std::is_array_v<T>                         ||
      std::is_same_v<T, int>                     ||
      std::is_same_v<T, char*>                   ||
      std::is_same_v<T, char const*>             ||
      std::is_same_v<T, std::string_view>
  );
  // clang-format on
};

} // namespace detail

// This enables formatting with fmt anything that has a to_str.
//
// The reason that we inherit from the std::string formatter is
// so that we can reuse its parser. Without the parser then we
// would not be able format custom types with non-trivial format
// strings.
template<base::Show S>
// clang-format off
requires( !detail::ExcludeFromFmtPromotion<std::remove_cvref_t<S>> )
struct formatter<S> : formatter<std::string> {
  // clang-format on
  template<typename FormatContext>
  auto format( S const& o, FormatContext& ctx ) {
    return formatter<std::string>::format( base::to_str( o ),
                                           ctx );
  }
};

} // namespace fmt