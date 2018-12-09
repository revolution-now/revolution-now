/****************************************************************
**util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Common utilities for all modules.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "aliases.hpp"
#include "errors.hpp"
#include "typed-int.hpp"

#include <algorithm>
#include <istream>
#include <optional>
#include <ostream>
#include <string_view>

namespace rn {

// Pass as the third template argument to hash map so that we can
// use enum classes as the keys in maps.
struct EnumClassHash {
  template<class T>
  std::size_t operator()( T t ) const {
    return static_cast<std::size_t>( t );
  }
};

ND int round_up_to_nearest_int_multiple( double d, int m );
ND int round_down_to_nearest_int_multiple( double d, int m );

// Get a reference to a value in a map. Since the key may not ex-
// ist, we return an optional. But since we want a reference to
// the object, we return an optional of a reference wrapper,
// since containers can't hold references. I think the reference
// wrapper returned here should only allow const references to be
// extracted.
template<typename KeyT, typename ValT,
         // typename... to allow for maps that may have
         // additional template parameters (but which we don't
         // care about here).
         template<typename KeyT_, typename ValT_, typename...>
         typename MapT>
ND OptCRef<ValT> get_val_safe( MapT<KeyT, ValT> const& m,
                               KeyT const&             k ) {
  auto found = m.find( k );
  if( found == m.end() ) return std::nullopt;
  return found->second;
}

// Gave up trying to make variadic templates work inside a
// templatized template parameter, so just use auto for the
// return value.
template<typename MapT, typename KeyT>
ND auto& get_val_or_die( MapT& m, KeyT const& k ) {
  auto found = m.find( k );
  CHECK( found != m.end() );
  return found->second;
}

// Gave up trying to make variadic templates work inside a
// templatized template parameter, so just use auto for the
// return value.
template<typename MapT, typename KeyT>
ND auto const& get_val_or_die( MapT const& m, KeyT const& k ) {
  auto found = m.find( k );
  CHECK( found != m.end() );
  return found->second;
}

// Non-const version.
template<typename KeyT, typename ValT,
         // typename... to allow for maps that may have
         // additional template parameters (but which we don't
         // care about here).
         template<typename KeyT_, typename ValT_, typename...>
         typename MapT>
ND OptRef<ValT> get_val_safe( MapT<KeyT, ValT>& m,
                              KeyT const&       k ) {
  auto found = m.find( k );
  if( found == m.end() ) return std::nullopt;
  return found->second;
}

// Does the set contain the given key. If not, returns nullopt.
// If so, returns the iterator to the location.
template<typename ContainerT, typename KeyT>
ND auto has_key( ContainerT const& s, KeyT const& k )
    -> std::optional<decltype( s.find( k ) )> {
  auto it = s.find( k );
  if( it == s.end() ) return std::nullopt;
  return it;
}

// Non-const version.
template<typename ContainerT, typename KeyT>
ND auto has_key( ContainerT& s, KeyT const& k )
    -> std::optional<decltype( s.find( k ) )> {
  auto it = s.find( k );
  if( it == s.end() ) return std::nullopt;
  return it;
}

template<typename ContainerT, typename ElemT>
ND int count( ContainerT& c, ElemT const& e ) {
  return std::count( c.begin(), c.end(), e );
}

} // namespace rn
