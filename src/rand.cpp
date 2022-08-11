/****************************************************************
**rand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-06.
*
* Description: Handles random numbers and all things random.
*
*****************************************************************/
#include "rand.hpp"

using namespace std;

namespace rn {

// namespace rng {
//
// void reseed( uint32_t seed ) {
//   CHECK( maybe_engine().has_value(),
//          "engine has not been seeded for the first time." );
//   rng::maybe_engine() = default_random_engine( seed );
// }
//
// } // namespace rng

/****************************************************************
** Rand
*****************************************************************/
Rand::Rand() : Rand( uint32_t( random_device{}() ) ) {}

Rand::Rand( uint32_t seed ) : engine_( seed ) {}

bool Rand::bernoulli( double p ) {
  CHECK( 0 <= p && p <= 1.0 );
  bernoulli_distribution biased_coin( p );
  return biased_coin( engine_ );
}

int Rand::between_ints( int lower, int upper, e_interval type ) {
  auto real_upper = upper;
  switch( type ) {
    case e_interval::closed: //
      CHECK_LE( lower, upper );
      break;
    case e_interval::half_open: //
      CHECK_LT( lower, upper );
      real_upper--;
      break;
  };
  uniform_int_distribution<int> uniform_dist( lower,
                                              real_upper );
  return uniform_dist( engine_ );
}

double Rand::between_doubles( double lower, double upper ) {
  uniform_real_distribution<double> uniform_dist( lower, upper );
  return uniform_dist( engine_ );
}

} // namespace rn
