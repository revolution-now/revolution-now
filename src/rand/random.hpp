/****************************************************************
**random.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Basic randomness facilities.
*
*****************************************************************/
#pragma once

// base
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <random>
#include <vector>

namespace rng {

/****************************************************************
** random
*****************************************************************/
struct random {
  random() : engine_( std::random_device{}() ) {}

  random( uint32_t const seed ) : engine_( seed ) {}

  template<typename RandomDevice>
  random( RandomDevice&& device )
    : engine_( std::forward<RandomDevice>( device )() ) {}

  void reseed( uint32_t new_seed );

  // Biased coin flip.
  [[nodiscard]] bool bernoulli( double p );

  // Closed on both ends.
  [[nodiscard]] int uniform( int lower, int upper );

  // Closed on both ends.
  [[nodiscard]] double uniform( double lower, double upper );

  // Uniform over all values of the type.
  template<typename T>
  requires std::is_integral_v<T>
  [[nodiscard]] T uniform() {
    return std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(),
        std::numeric_limits<T>::max() )( engine_ );
  }

  template<typename T>
  base::maybe<T const&> pick_one_safe(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    if( v.empty() ) return base::nothing;
    return v[uniform( 0, v.size() - 1 )];
  }

  template<typename T>
  T const& pick_one(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    UNWRAP_CHECK( res, pick_one_safe( v ) );
    return res;
  }

 private:
  // It seems the default engine (linear congruential) does not
  // produce high-enough quality results. Given that there has
  // always been so much talk about issues with the random number
  // generator in the OG, we definitely don't want to risk any
  // issues there in the NG, and this Mersenne Twister should
  // guarantee that.
  using engine_t = std::mt19937;

  engine_t engine_;
};

} // namespace rng
