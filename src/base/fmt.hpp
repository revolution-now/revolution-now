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
#include <source_location>

// {fmt}
// We should only include this file from this header so that we
// can control which warnings are supressed.
#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"
#endif
#include "fmt/format.h"
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

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
  consteval FmtStrAndLoc( char const*          s,
                          std::source_location loc =
                              std::source_location::current() )
    : fs( s ), loc( loc ) {}
  fmt::format_string<Args...> fs;
  std::source_location        loc;
};

} // namespace base
