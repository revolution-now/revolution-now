/****************************************************************
**query-enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-07.
*
* Description: Helpers for querying info about reflected enums.
*
*****************************************************************/
#pragma once

// refl
#include "ext.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <array>
#include <concepts>

#define FOR_ENUM( var, enum_type ) \
  for( enum_type const var : refl::enum_values<enum_type> )

namespace refl {

template<ReflectedEnum E>
constexpr int enum_count = traits<E>::value_names.size();

template<ReflectedEnum E>
inline constexpr auto enum_values = [] {
  std::array<E, enum_count<E>> arr{};
  for( int i = 0; i < enum_count<E>; ++i )
    arr[i] = static_cast<E>( i );
  return arr;
}();

template<ReflectedEnum E>
constexpr std::string_view enum_value_name( E val ) {
  return traits<E>::value_names[static_cast<size_t>( val )];
}

template<ReflectedEnum E, std::integral Int>
constexpr base::maybe<E> enum_from_integral( Int val ) {
  base::maybe<E> res;
  int            intval = static_cast<int>( val );
  if( intval < 0 || intval >= enum_count<E> ) return res;
  res = static_cast<E>( intval );
  return res;
}

template<ReflectedEnum E>
constexpr base::maybe<E> enum_from_string(
    std::string_view name ) {
  base::maybe<E> res;
  int            i = 0;
  for( std::string_view sv : traits<E>::value_names ) {
    if( name == sv ) {
      res = static_cast<E>( i );
      break;
    }
    ++i;
  }
  return res;
}

} // namespace refl
