/****************************************************************
**maybe-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-30.
*
* Description: Utilities for working with `maybe` types.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"

// C++ standard library
#include <vector>

namespace base {

// This will take the vectors of maybes and will gather all of
// them that are not nullopt and move their values into a vector
// and return it.
template<typename T>
std::vector<T> cat_maybes( std::vector<maybe<T>>&& ms ) {
  std::vector<T> res;
  // We might need up to this size.
  res.reserve( ms.size() );
  for( maybe<T>& m : ms )
    if( m ) res.push_back( std::move( *m ) );
  return res;
}

// This will take the vectors of maybes and will gather all of
// them that are not nullopt and move their values into a vector
// and return it.
template<typename T>
std::vector<T> cat_maybes( std::vector<maybe<T>> const& ms ) {
  std::vector<T> res;
  // We might need up to this size.
  res.reserve( ms.size() );
  for( maybe<T> const& m : ms )
    if( m ) res.push_back( *m );
  return res;
}

// Does a dynamic cast with references, but returns a maybe so
// that we can fail by returning nothing if the dynamic cast
// fails.
template<typename To, typename From>
maybe<To&> maybe_dynamic_cast( From& from ) {
  constexpr bool is_lvalue_ref =
      std::is_lvalue_reference_v<decltype( from )>;
  static_assert( is_lvalue_ref );
  using To_noref_t = std::remove_reference_t<To>;
  if( To_noref_t* to = dynamic_cast<To_noref_t*>( &from ) )
    return *to;
  else
    return nothing;
}

} // namespace base