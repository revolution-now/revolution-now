/****************************************************************
**ranges.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-12.
*
* Description: Utilities for ranges.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fmt-helper.hpp"

// range-v3
#include "range/v3/all.hpp"

namespace rv = ::ranges::view;
namespace rg = ::ranges;

namespace rn {

// This behaves just like the take_while function except that it
// will also include the element that stops the "taking," if
// there is one.
template<typename Func>
auto take_while_inclusive( Func f ) {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using Rng = decltype( r );
    return rv::concat( std::forward<Rng>( r ) //
                           | rv::take_while( f ),
                       std::forward<Rng>( r )    //
                           | rv::drop_while( f ) //
                           | rv::take( 1 ) );
  } );
}

template<typename Rng>
std::string rng_to_string( Rng&& rng ) {
  return ranges::accumulate(
      std::forward<Rng>( rng )                           //
          | rv::transform( L( fmt::format( "{}", _ ) ) ) //
          | rv::intersperse( std::string( "," ) ),
      std::string{} );
}

// Given a function f and a range [1,2,3...] this will return a
// range of: [(1,f(1)), (2,f(2)), (3,f(3)), ...]
template<typename Func>
auto transform_pair( Func f ) {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using Rng = decltype( r );
    return rv::zip(
        std::forward<Rng>( r ),
        std::forward<Rng>( r ) | rv::transform( f ) );
  } );
}

// This function applies `f` to each element in the range to
// derive keys, then finds the minimum key, and returns the range
// value corresponding to that key.
template<typename Func>
auto min_by_key( Func f ) {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using ResType = decltype( *r.begin() );
    using KeyType = decltype( f( *r.begin() ) );
    using Rng     = decltype( r );
    std::optional<ResType> res{};
    std::optional<KeyType> min_key{};
    for( auto const& [elem, key] :
         std::forward<Rng>( r ) | transform_pair( f ) ) {
      if( !min_key.has_value() || key < *min_key ) {
        min_key = key;
        res     = elem;
      }
    }
    return res;
  } );
}

} // namespace rn
