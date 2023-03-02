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

#include "config.hpp"

// base
#include "attributes.hpp"
#include "maybe.hpp"

namespace base {

template<typename Container, typename Key>
concept HasBuiltinFind = requires( Container c, Key k ) {
  { c.find( k ) };
};

// Container-based std::find.
template<typename Container, typename Key>
requires( !HasBuiltinFind<std::remove_cvref_t<Container>, Key> )
auto find( Container&& s ATTR_LIFETIMEBOUND, Key const& k ) {
  return std::find( std::begin( s ), std::end( s ), k );
}

// Same as above but utilizes the builtin find member if the con-
// tainer has it.
template<typename Container, typename Key>
requires HasBuiltinFind<std::remove_cvref_t<Container>, Key>
auto find( Container&& s ATTR_LIFETIMEBOUND, Key const& k ) {
  if( auto it = std::forward<Container>( s ).find( k );
      it != std::forward<Container>( s ).end() )
    return it;
  return s.end();
}

// Does the container contain the given key. If not, returns end
// of range. If so, returns the iterator to the location.
template<typename ContainerT, typename KeyT>
requires requires( ContainerT const& c, KeyT const& k ) {
  {
    c.find( k )
  } -> std::same_as<typename ContainerT::const_iterator>;
}
auto find( ContainerT&& s ATTR_LIFETIMEBOUND, KeyT const& k ) {
  if( auto it = std::forward<ContainerT>( s ).find( k );
      it != std::forward<ContainerT>( s ).end() )
    return it;
  return s.end();
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