/****************************************************************
**macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-28.
*
* Description: General macros used throughout the code.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// TODO: When C++20 comes change this to the new [[unreachable]].
#ifndef _MSC_VER
// POSIX.
#  define UNREACHABLE_LOCATION __builtin_unreachable()
#else
// MSVC.
#  define UNREACHABLE_LOCATION __assume( false )
#endif

// Hopefully, if someone else defines this, it will be defined
// equivalently, since this seems to be a standard thing.
#ifndef FWD
#  define FWD( ... ) \
    ::std::forward<decltype( __VA_ARGS__ )>( __VA_ARGS__ )
#endif

#define CALLER_LOCATION( var ) \
  const base::SourceLoc& var = base::SourceLoc::current()
