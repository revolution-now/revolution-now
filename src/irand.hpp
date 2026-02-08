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

// rand
#include "rand/entropy.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"

// C++ standard library
#include <array>
#include <map>
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

  virtual void reseed( rng::entropy const& seed ) = 0;

  // Biased coin flip.  Returns true with probability p.
  [[nodiscard]] virtual bool bernoulli( double p ) = 0;

  // Random integer between tbe bounds, closed on both ends.
  [[nodiscard]] virtual int uniform_int( int lower,
                                         int upper ) = 0;

  // Random floating point number in [lower, upper).
  [[nodiscard]] virtual double uniform_double(
      double lower, double upper ) = 0;

  [[nodiscard]] virtual double normal( double mean,
                                       double stddev ) = 0;

  // This generates a seed (entropy) object just using the cur-
  // rent state of the pseudo-random generator; no true random-
  // ness here. This would be used to generate e.g. map genera-
  // tion seeds once the rng itself has been seeded.
  [[nodiscard]] virtual rng::seed
  generate_deterministic_seed() = 0;

  /**************************************************************
  ** For convenience.
  ***************************************************************/
  // Shuffles the elements. Vector can be empty.
  template<typename T>
  void shuffle( std::vector<T>& vec );

  template<typename T, size_t N>
  void shuffle( std::array<T, N>& arr );

  // Vector must be non-empty. Picks a random element.
  template<typename T>
  [[nodiscard]] T const& pick_one(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    CHECK( !v.empty() );
    return v[uniform_int( 0, v.size() - 1 )];
  }

  // Picks a random value given the weights. All weights should
  // be >= 0. A zero weight means that the value will never be
  // chosen.
  template<typename T>
  [[nodiscard]] T pick_from_weighted_values(
      std::map<T, int> const& weights );

  // For doubles.
  template<typename T>
  [[nodiscard]] T pick_from_weighted_values(
      std::map<T, double> const& weights );
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
    int source = uniform_int( i, last_idx );
    using std::swap;
    swap( vec[i], vec[source] );
  }
}

template<typename T, size_t N>
void IRand::shuffle( std::array<T, N>& arr ) {
  if constexpr( N == 0 )
    return;
  else {
    int constexpr kLastIdx = N - 1;
    // i < last_idx because we don't want to consider swapping
    // the last element with itself, which would have not pur-
    // pose.
    for( int i = 0; i < kLastIdx; ++i ) {
      int source = uniform_int( i, kLastIdx );
      using std::swap;
      swap( arr[i], arr[source] );
    }
  }
}

template<typename T>
T IRand::pick_from_weighted_values(
    std::map<T, int> const& weights ) {
  int total = 0;
  for( auto const& [item, weight] : weights ) total += weight;
  CHECK_GT( total, 0 );
  int const stop = uniform_int( 0, total - 1 );
  int running    = 0;
  for( auto const& [item, weight] : weights ) {
    running += weight;
    if( running > stop ) return item;
  }
  SHOULD_NOT_BE_HERE;
}

template<typename T>
T IRand::pick_from_weighted_values(
    std::map<T, double> const& weights ) {
  double total = 0;
  for( auto const& [item, weight] : weights ) total += weight;
  CHECK_GE( total, 0.0 );
  double const stop = uniform_double( 0.0, total );
  double running    = 0.0;
  T res             = {};
  for( auto const& [item, weight] : weights ) {
    res = item;
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

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::IRand, owned_by_cpp ){};

}
