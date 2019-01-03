/****************************************************************
**enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-03.
*
* Description: Helpers for dealing with enums.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// better-enums
#include "better-enums/enum.h"

// This creates a reflected enum class.  Use it like this:
//
// enum class e_(color,
//   red, green, blue
// );
//
// This will produce an enum type that:
//
//   * Has a memory footprint of just an int
//   * Works in switch statements with compiler case checking
//   * Supports iteration over all values
//   * Can be converted to/from strings
//   * Can be automatically logged as its string names
//
// Iteration and logging "just works" as follows:
//
//   for( auto val : values<e_color> )
//     logger->debug( "val: {}", val );
//
#define e_( n, ... )                      \
  __e_##n;                                \
  BETTER_ENUM( e_##n, int, __VA_ARGS__ ); \
  template<>                              \
  auto values_impl<e_##n>() {             \
    return e_##n::_values();              \
  }

namespace rn {

template<typename Enum>
auto values_impl();

template<typename Enum>
auto values = values_impl<Enum>();

// Verify that the footprint of the enums is just that of an int.
enum class e_( size_test, _ );
static_assert( sizeof( e_size_test ) == sizeof( int ) );

} // namespace rn
