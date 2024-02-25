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

int Rand::between_ints( int lower, int upper ) {
  CHECK_LE( lower, upper );
  uniform_int_distribution<int> uniform_dist( lower, upper );
  return uniform_dist( engine_ );
}

double Rand::between_doubles( double lower, double upper ) {
  uniform_real_distribution<double> uniform_dist( lower, upper );
  return uniform_dist( engine_ );
}

} // namespace rn
