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

} // namespace rn
