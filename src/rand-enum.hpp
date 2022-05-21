/****************************************************************
**rand-enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-21.
*
* Description: Helpers for picking random enum values.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum-map.hpp"
#include "rand.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/error.hpp"

namespace rn::rng {

// Given a reflected enum type it will return a random value out
// of all the possible values.
template<refl::ReflectedEnum Enum>
Enum pick_one() {
  constexpr auto count = refl::enum_count<Enum>;
  static_assert( count > 0 );
  auto idx = between( 0, count, e_interval::half_open );
  return refl::enum_values<Enum>[idx];
}

// Picks a random enum value given the weights. All weights
// should be >= 0. A zero weight means that the enum value will
// never be chosen. Must be a reflected enum because the EnumMap
// requires this.
template<refl::ReflectedEnum T>
T pick_from_weighted_enum_values(
    EnumMap<T, int> const& weights ) {
  int total = 0;
  for( auto [item, weight] : weights ) total += weight;
  CHECK_GE( total, 0 );
  int stop    = between( 0, total, rng::e_interval::half_open );
  int running = 0;
  for( auto [item, weight] : weights ) {
    running += weight;
    if( running > stop ) return item;
  }
  SHOULD_NOT_BE_HERE;
}

} // namespace rn::rng
