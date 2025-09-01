/****************************************************************
**generator-combinators.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-01.
*
* Description: Combinators for generators.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "generator.hpp"

namespace base {

// Chain a list of ranges. Returns a generator that, when iter-
// ated, yields each element in each range in order. Example:
//
//   auto const range1 = vector<string>{ ... };
//   auto const range2 = list<string>{ ... };
//   auto const range3 = array<string, 4>{ ... };
//
//   auto const chained = range_concat( range1, range2, range3 );
//
//   for( string const& e : chained )
//     ...
//
template<typename First, typename... Rest>
requires( std::is_same_v<typename First::value_type,
                         typename Rest::value_type> &&
          ... )
auto range_concat( First const& rng1 ATTR_LIFETIMEBOUND,
                   Rest const&... rngs ATTR_LIFETIMEBOUND )
    -> generator<typename First::value_type> {
  using R = typename First::value_type;
  static auto const yield_range =
      []( auto const& rng ATTR_LIFETIMEBOUND ) -> generator<R> {
    for( auto const& e : rng ) co_yield e;
  };
  for( generator<R> const& rng :
       { yield_range( rng1 ), yield_range( rngs )... } )
    for( auto const& e : rng ) //
      co_yield e;
}

} // namespace base
