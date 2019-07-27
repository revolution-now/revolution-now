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

// Place this into a struct to add the necessary members to make
// it useful as a key in a cache. Must be at the very bottom of
// the struct. Will static_assert that no fields have been left
// out.
#define MAKE_CACHE_KEY( name, ... )                            \
  auto to_tuple() const { return std::tuple{__VA_ARGS__}; }    \
                                                               \
  template<typename H>                                         \
  friend H AbslHashValue( H h, name const& c ) {               \
    return H::combine( std::move( h ), c.to_tuple() );         \
  }                                                            \
                                                               \
  friend bool operator==( name const& lhs, name const& rhs ) { \
    return lhs.to_tuple() == rhs.to_tuple();                   \
  }                                                            \
  }                                                            \
  ;                                                            \
  static_assert( sizeof( decltype( name{}.to_tuple() ) ) ==    \
                 sizeof( name ) );                             \
  struct ____##name {
