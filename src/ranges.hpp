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

template<typename Range, typename Func>
auto take_while_inclusive( Range const& r, Func f ) {
  return rv::concat( r | rv::take_while( f ),
                     r | rv::drop_while( f ) | rv::take( 1 ) );
}

template<typename Rng>
std::string rng_to_string( Rng rng ) {
  return ranges::accumulate(
      rng | rv::transform( L( fmt::format( "{}", _ ) ) ) //
          | rv::intersperse( std::string( ", " ) ),
      std::string{} );
}

} // namespace rn
