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
#include "attributes.hpp"
#include "error.hpp"
#include "maybe.hpp"

// C++ standard library
#include <random>
#include <vector>

namespace base {

/****************************************************************
** random
*****************************************************************/
struct random {
  random() : engine_( std::random_device{}() ) {}

  random( uint32_t seed ) : engine_( seed ) {}

  template<typename RandomDevice>
  random( RandomDevice&& device )
    : engine_( std::forward<RandomDevice>( device )() ) {}

  bool bernoulli( double p );

  // Closed on both ends.
  int uniform( int lower, int upper );

  // Closed on both ends.
  double uniform( double lower, double upper );

  template<typename T>
  maybe<T const&> pick_one_safe(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    if( v.empty() ) return nothing;
    std::uniform_int_distribution<int> uniform_dist(
        0, v.size() - 1 );
    return v[uniform_dist( engine_ )];
  }

  template<typename T>
  T const& pick_one(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    UNWRAP_CHECK( res, pick_one_safe( v ) );
    return res;
  }

 private:
  std::default_random_engine engine_;
};

} // namespace base
