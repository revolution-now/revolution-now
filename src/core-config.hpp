/****************************************************************
**core-config.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-09.
*
* Description: Core declarations/macros/config for all code
*              files.
*
*****************************************************************/
#pragma once

#define ND [[nodiscard]]

// The DEBUG_RELEASE macro will evalutate to the first argument
// if this is a debug build and the second otherwise.
#ifdef NDEBUG // release build
#  define DEBUG_RELEASE( a, b ) ( b )
#else // debug build
#  define DEBUG_RELEASE( a, b ) ( a )
#endif

#ifdef NDEBUG // release build
#  define WHEN_DEBUG( a )
#else // debug build
#  define WHEN_DEBUG( a ) ( a )
#endif

// We use ... to allow types with commas in them.
#define NOTHROW_MOVE( ... )                                \
  static_assert(                                           \
      std::is_nothrow_move_constructible_v<__VA_ARGS__> ); \
  static_assert( std::is_nothrow_move_assignable_v<__VA_ARGS__> )
