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

// C++ standard library
#include <string>
#include <string_view>

namespace base {

void to_str( int const& o, std::string& out );
void to_str( std::string_view o, std::string& out );

// Only use this one when you know that you're only converting a
// single value to a string. Otherwise, prefer the to_str variant
// with an output argument because it reuses the same string ob-
// ject for efficiency.
template<typename T>
std::string to_str( T&& o ) {
  std::string res;
  to_str( o, res );
  return res;
}

} // namespace base