/****************************************************************
**rand.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-06.
*
* Description: Handles random numbers and all things random.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"

// Rds
#include "rds/helper/enum.hpp"

// C++ standard library
#include <algorithm>
#include <array>
#include <random>
#include <vector>

namespace rn {

namespace rng {

std::default_random_engine& engine();

// Flip a fair coin.
bool flip_coin();

// p is probability of getting true, must be between [0,1.0].
bool flip_coin( double p );

// half_open means [a,b), closed means [a,b].
enum class e_interval { half_open, closed };

// Random integer between tbe bounds, where the meaning of "be-
// tween" depends on the interval type. If interval is half open
// then lower must be < upper.
int between( int lower, int upper, e_interval type );

// Given a reflected enum type it will return a random value out
// of all the possible values.
template<typename Enum>
Enum pick_one() {
  constexpr auto count = enum_traits<Enum>::count;
  static_assert( count > 0 );
  auto idx = between( 0, count, e_interval::half_open );
  return enum_traits<Enum>::values[idx];
}

// Pick a random enum value out of a set of choices. Use like
// this:
//
//   auto val = rng::pick_one( array{e_test::b, e_test::d} );
//
// Since this function is constexpr it will require that the ar-
// gument be constexpr, so there will be no runtime overhead in
// constructing it.
template<typename T, size_t N>
constexpr T const& pick_one( std::array<T, N> const& arr ) {
  static_assert( N > 0 );
  return arr[between( 0, N, e_interval::half_open )];
}

// Vector must be non-empty.  Picks a random element.
template<typename T>
T const& pick_one( std::vector<T> const& v ) {
  return v[between( 0, v.size(), e_interval::half_open )];
}

template<typename T>
void shuffle( std::vector<T>& v ) {
  std::shuffle( v.begin(), v.end(), engine() );
}

/****************************************************************
** Testing
*****************************************************************/
// This only needs to be called to control seeding during
// testing.
void reseed( uint32_t seed );

} // namespace rng

} // namespace rn
