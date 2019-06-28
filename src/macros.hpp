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

#define ASSERT_NOTHROW_MOVING( type )                          \
  static_assert( std::is_nothrow_move_constructible_v<type> ); \
  static_assert( std::is_nothrow_move_assignable_v<type> )

// TODO: When C++20 comes change this to the new [[unreachable]].
#ifndef _MSC_VER
// POSIX.
#  define UNREACHABLE_LOCATION __builtin_unreachable()
#else
// MSVC.
#  define UNREACHABLE_LOCATION __assume( false )
#endif
