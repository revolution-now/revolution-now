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

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <concepts>
#include <string>
#include <string_view>

namespace base {

template<typename T>
concept Show = requires( T o, std::string s ) {
  { to_str( o, s ) } -> std::same_as<void>;
};

// ADL is not going to find these, may have to fix this at some
// point.
void to_str( int const& o, std::string& out );
void to_str( std::string_view o, std::string& out );

// Only use this one when you know that you're only converting a
// single value to a string. Otherwise, prefer the to_str variant
// with an output argument because it reuses the same string ob-
// ject for efficiency.
template<Show T>
std::string to_str( T&& o ) {
  std::string res;
  to_str( o, res );
  return res;
}

} // namespace base

namespace fmt {

// This enables formatting with fmt anything that has a to_str.
//
// The reason that we inherit from the std::string formatter is
// so that we can reuse its parser. Without the parser then we
// would not be able format custom types with non-trivial format
// strings.
template<base::Show S>
struct formatter<S> : formatter<std::string> {
  template<typename FormatContext>
  auto format( S const& o, FormatContext& ctx ) {
    return formatter<std::string>::format( base::to_str( o ),
                                           ctx );
  }
};

} // namespace fmt