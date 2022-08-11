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
#include "irand.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"

// base
#include "base/error.hpp"

namespace rn {

// Given a reflected enum type it will return a random value out
// of all the possible values.
template<refl::ReflectedEnum Enum>
Enum pick_one( IRand& rand ) {
  constexpr auto count = refl::enum_count<Enum>;
  static_assert( count > 0 );
  auto idx = rand.between_ints( 0, count,
                                IRand::e_interval::half_open );
  return refl::enum_values<Enum>[idx];
}

// Picks a random enum value given the weights. All weights
// should be >= 0. A zero weight means that the enum value will
// never be chosen. Must be a reflected enum because the
// refl::enum_map requires this.
template<refl::ReflectedEnum T>
T pick_from_weighted_enum_values(
    IRand& rand, refl::enum_map<T, int> const& weights ) {
  int total = 0;
  for( auto [item, weight] : weights ) total += weight;
  CHECK_GE( total, 0 );
  int stop    = rand.between_ints( 0, total,
                                   IRand::e_interval::half_open );
  int running = 0;
  // This iteration needs to be in order of enum values, and not
  // the default iteration of the enum_map container which uses
  // an unordered_map as a backing container.
  for( T item : refl::enum_values<T> ) {
    int const weight = weights[item];
    running += weight;
    if( running > stop ) return item;
  }
  SHOULD_NOT_BE_HERE;
}

// For doubles.
template<refl::ReflectedEnum T>
T pick_from_weighted_enum_values(
    IRand& rand, refl::enum_map<T, double> const& weights ) {
  double total = 0;
  for( auto [item, weight] : weights ) total += weight;
  CHECK_GE( total, 0.0 );
  double stop    = rand.between_doubles( 0.0, total );
  double running = 0.0;
  T      res     = {};
  // This iteration needs to be in order of enum values, and not
  // the default iteration of the enum_map container which uses
  // an unordered_map as a backing container.
  for( T item : refl::enum_values<T> ) {
    double const weight = weights[item];
    res                 = item;
    running += weight;
    if( running > stop ) break;
  }
  // Unlike with the int version above, I am not convinced that
  // we can check-fail if we get here in the case of doubles,
  // since I'm not sure if there are any edge cases with rounding
  // errors. So instead we'll just arrange to always return a
  // value.
  return res;
}

} // namespace rn
