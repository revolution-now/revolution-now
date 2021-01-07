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