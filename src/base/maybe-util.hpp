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

// base
#include "maybe.hpp"

// C++ standard library
#include <vector>

namespace base {

template<typename T>
constexpr bool is_maybe_v = false;
template<typename T>
constexpr bool is_maybe_v<maybe<T>> = true;

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

} // namespace base