/****************************************************************
**source-loc.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-13.
*
* Description: Include this to use the source_location facility.
*
*****************************************************************/
#pragma once

#include "config.hpp"

#include <type_traits>

// Instructions:
//
// To use source_location, just the SourceLoc alias instaed,
// which will ensure that it compiles with libraries that don't
// yet support it by way of the dummy stand-in below. When all
// supported libraries have it then this can be removed.

// source_location (coming in C++20)
#if __has_include( <experimental/source_location>)

#  include <experimental/source_location>
namespace base {
using SourceLoc = std::experimental::source_location;
}

#else

namespace base {
// Dummy to allow compilation to proceed; can be deleted after
// all compilers supported support C++20.
struct SourceLoc {
  int         line() const { return 0; }
  int         column() const { return 0; }
  char const* file_name() const { return "unknown"; }
  char const* function_name() const { return "unknown"; }

  static constexpr SourceLoc current() { return SourceLoc{}; }
};
static_assert( std::is_nothrow_move_constructible_v<SourceLoc> );
static_assert( std::is_nothrow_move_assignable_v<SourceLoc> );
} // namespace base

#endif