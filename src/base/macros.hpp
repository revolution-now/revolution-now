/****************************************************************
**macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Macros.
*
*****************************************************************/
#pragma once

#include "config.hpp"

#define BASE_IDENTITY( ... ) __VA_ARGS__

// TODO: remove this guard once we no longer depend on
// base-util/macros.hpp.
#ifndef TO_STRING
#  define TO_STR1NG( x ) #  x
#  define TO_STRING( x ) TO_STR1NG( x )
#endif

// TODO: remove this guard once we no longer depend on
// base-util/macros.hpp.
#ifndef STRING_JOIN
#  define STRING_JO1N( arg1, arg2 ) arg1##arg2
#  define STRING_JOIN( arg1, arg2 ) STRING_JO1N( arg1, arg2 )
#endif

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
