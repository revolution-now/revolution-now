/****************************************************************
**irand.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: Injectable interface for random number generation.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"

// C++ standard library
#include <vector>

namespace rn {

/****************************************************************
** IRand
*****************************************************************/
// Anything in the game that needs to generate random behavior
// and that also needs to be unit tested should use this inter-
// face for all random number generation.
struct IRand {
  virtual ~IRand() = default;

  // Biased coin flip.  Returns true with probability p.
  [[nodiscard]] virtual bool bernoulli( double p ) = 0;

  // Random integer between tbe bounds, closed on both ends.
  [[nodiscard]] virtual int between_ints( int lower,
                                          int upper ) = 0;

  // Random floating point number in [lower, upper).
  [[nodiscard]] virtual double between_doubles(
      double lower, double upper ) = 0;

  /**************************************************************
  ** For convenience.
  ***************************************************************/
  // Shuffles the elements. Vector can be empty. Picks a random
  // element.
  template<typename T>
  void shuffle( std::vector<T>& vec );

  // Vector must be non-empty. Picks a random element.
  template<typename T>
  [[nodiscard]] T const& pick_one(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    CHECK( !v.empty() );
    return v[between_ints( 0, v.size() - 1 )];
  }

  // Picks a random value given the weights. All weights should
  // be >= 0. A zero weight means that the value will never be
  // chosen.
  template<typename T>
  [[nodiscard]] T pick_from_weighted_values(
      std::vector<std::pair<T, int>> const& weights );

  // For doubles.
  template<typename T>
  [[nodiscard]] T pick_from_weighted_values(
      std::vector<std::pair<T, double>> const& weights );
};

void to_str( IRand const& o, std::string& out,
             base::tag<IRand> );

template<typename T>
void IRand::shuffle( std::vector<T>& vec ) {
  if( vec.empty() ) return;
  int const last_idx = vec.size() - 1;
  // i < last_idx because we don't want to consider swapping the
  // last element with itself, which would have not purpose.
  for( int i = 0; i < last_idx; ++i ) {
    int source = between_ints( i, last_idx );
    using std::swap;
    swap( vec[i], vec[source] );
  }
}

template<typename T>
T IRand::pick_from_weighted_values(
    std::vector<std::pair<T, int>> const& weights ) {
  int total = 0;
  for( auto [item, weight] : weights ) total += weight;
  CHECK_GT( total, 0 );
  int stop    = between_ints( 0, total - 1 );
  int running = 0;
  for( auto const& [item, weight] : weights ) {
    running += weight;
    if( running > stop ) return item;
  }
  SHOULD_NOT_BE_HERE;
}

template<typename T>
T IRand::pick_from_weighted_values(
    std::vector<std::pair<T, double>> const& weights ) {
  double total = 0;
  for( auto [item, weight] : weights ) total += weight;
  CHECK_GE( total, 0.0 );
  double stop    = between_doubles( 0.0, total );
  double running = 0.0;
  T      res     = {};
  for( auto const& [item, weight] : weights ) {
    res = item;
    running += weight;
    if( running > stop ) break;
  }
  // Unlike with the int version above, I am not convinced that
  // we can check-fail if we get here in the case of doubles,
  // since I'm not sure if there are any edge cases with
  // rounding errors. So instead we'll just arrange to always
  // return a value.
  return res;
}

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::IRand, owned_by_cpp ){};

}
