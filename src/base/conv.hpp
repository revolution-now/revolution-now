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
#include <concepts>
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
maybe<double> stod( std::string const& s );

// This is to replace std::from_chars -- it will enforce that the
// input string is not empty and that the parsing consumes the
// entire string.
template<std::integral T>
maybe<T> from_chars( std::string_view sv,
                     int              base = default_base ) {
  maybe<T>               res{ T{ 0 } };
  std::from_chars_result fc_res =
      std::from_chars( sv.begin(), sv.end(), *res, base );
  if( fc_res.ec != std::errc{} || fc_res.ptr != sv.end() )
    res.reset();
  return res;
}

template<std::floating_point T>
maybe<T> from_chars( std::string_view sv ) {
  // FIXME: uncomment this once libc++ supports it.
  // maybe<T> res{ T{ 0 } };
  // std::from_chars_result fc_res =
  //     std::from_chars( sv.begin(), sv.end(), *res );
  // if( fc_res.ec != std::errc{} || fc_res.ptr != sv.end() )
  //   res.reset();
  // return res;
  return base::stod( std::string( sv ) );
}

// Guidelines say that, when writing a number in the middle of a
// sentence, that it should be written out when it is 0-9, and
// otherwise the numeral can be used. However, note that if the
// number is at the beginning of the sentence then it always
// should be written out, which this function does not support.
std::string int_to_string_literary( int n );

} // namespace base