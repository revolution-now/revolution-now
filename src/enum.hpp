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

// Hopefully make things lighter by removing exception throwing
// from the reflected enums. This will remove any functions from
// the API that throw.
#define BETTER_ENUMS_NO_EXCEPTIONS 1

// In order to tell better-enums that we want a default
// constructor we need to actually write it in this macro.
#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR( Enum ) \
public:                                          \
  constexpr Enum() : _value( 0 ) {}

// better-enums.  NOTE: this should be the only place in the
// code base that we include this header.
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
#define e_( n, ... )                                   \
  __e_##n;                                             \
  BETTER_ENUM( e_##n, int, __VA_ARGS__ );              \
  template<typename H>                                 \
  H AbslHashValue( H state, e_##n const& o ) {         \
    return H::combine( std::move( state ), o._value ); \
  }

namespace rn {

template<typename Enum>
constexpr auto values = Enum::_values();

// Verify that the footprint of the enums is just that of an int.
enum class e_( size_test, _ );
static_assert( sizeof( e_size_test ) == sizeof( int ) );

char const* enum_to_display_name_impl(
    std::string const& type_name, int index,
    char const* default_ );

template<typename ReflectedEnum>
char const* enum_to_display_name( ReflectedEnum value ) {
  auto const* default_ = value._to_string();
  return enum_to_display_name_impl(
      ReflectedEnum::_name(), value._to_index(), default_ );
}

} // namespace rn
