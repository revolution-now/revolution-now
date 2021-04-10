/****************************************************************
**keyval.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: Utilities for looking things up in maps.
*
*****************************************************************/
#pragma once

// base
#include "attributes.hpp"
#include "maybe.hpp"

namespace base {

// Does the set contain the given key. If not, returns nullopt.
// If so, returns the iterator to the location.
template<typename ContainerT, typename KeyT>
auto find( ContainerT&& s ATTR_LIFETIMEBOUND, KeyT const& k ) {
  maybe<decltype( std::forward<ContainerT>( s ).find( k ) )> res;
  if( auto it = std::forward<ContainerT>( s ).find( k );
      it != std::forward<ContainerT>( s ).end() )
    res = it;
  return res;
}

// Get a reference to a value in a map. Since the key may not ex-
// ist, we return an optional.
template<typename MapT, typename KeyT>
auto lookup( MapT&& m ATTR_LIFETIMEBOUND, KeyT const& k )
    -> maybe<decltype( std::forward<MapT>( m ).at( k ) )> {
  if( auto found = std::forward<MapT>( m ).find( k );
      found != m.end() )
    return found->second;
  return nothing;
}

} // namespace base