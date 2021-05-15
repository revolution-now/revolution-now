/****************************************************************
**conv.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: Utilities for conversions between numbers and
*              strings.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"

// C++ standard library
#include <charconv>
#include <string>

namespace base {

/****************************************************************
** From-String utilities
*****************************************************************/
inline constexpr int default_base{ 10 }; // base 10 is decimal

// This is to replace std::stoi -- it will enforce that the input
// string is not empty and that the parsing consumes the entire
// string.
maybe<int> stoi( std::string const& s, int base = default_base );

// This is to replace std::from_chars for integers -- it will en-
// force that the input string is not empty and that the parsing
// consumes the entire string.
template<typename Integral>
maybe<Integral> from_chars( std::string_view sv,
                            int base = default_base ) {
  maybe<Integral>        res{ Integral{ 0 } };
  std::from_chars_result fc_res =
      std::from_chars( sv.begin(), sv.end(), *res, base );
  if( fc_res.ec != std::errc{} || fc_res.ptr != sv.end() )
    res.reset();
  return res;
}

} // namespace base