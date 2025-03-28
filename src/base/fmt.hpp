/****************************************************************
**fmt.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: Just includes fmt.  Probably don't need this.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <format>
#include <source_location>

namespace fmt {

// TODO: temporary to ease migration from fmt::format to std::-
// format. We should only be using the std:: ones in new code,
// and eventually we should migrate callers to use std:: on ex-
// isting fmt:: calls.
using ::std::format;
using ::std::format_context;
using ::std::format_error;
using ::std::format_string;
using ::std::formatter;
using ::std::to_string;

// TODO: remove once we have std::print.
template<typename... Args>
inline void print( std::format_string<Args...> const fmt,
                   Args&&... args ) {
  std::printf(
      "%s",
      std::vformat( fmt.get(), std::make_format_args( args... ) )
          .c_str() );
}

// TODO: remove once we have std::println.
template<typename... Args>
inline void println( std::format_string<Args...> const fmt,
                     Args&&... args ) {
  std::printf(
      "%s\n",
      std::vformat( fmt.get(), std::make_format_args( args... ) )
          .c_str() );
}

} // namespace fmt

namespace base {

/****************************************************************
** FmtStrAndLoc
*****************************************************************/
// This class is a helper that is implicitly constructed from a
// constexpr string, but also captures the source location in the
// process. It is used to automatically collect source location
// info when logging. We can't use the usual technique of making
// a defaulted source_location parameter at the end of the argu-
// ment list of the logging statements because they already need
// to have a variable number of arguments to support formatting.
// Note that we also store the format string in a format_string
// so that we get fmt's compile time format checking, which we
// would otherwise lose. We really want the compile-time format
// string checking afforded to us by fmt::format_string because
// otherwise format string syntax issues (or argument count dis-
// crepencies) would not be caught until runtime at which point
// fmt throws an exception which immediately terminates our pro-
// gram without much info for debugging where it happened. So
// even though we could bypass the compile time checking by using
// fmt::runtime(...) and perhaps save some compile time, we opt
// to keep the checking.
template<typename... Args>
struct FmtStrAndLoc {
  consteval FmtStrAndLoc( char const* s,
                          std::source_location loc =
                              std::source_location::current() )
    : fs( s ), loc( loc ) {}
  fmt::format_string<Args...> fs;
  std::source_location loc;
};

} // namespace base
