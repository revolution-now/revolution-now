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
Rand::Rand( uint32_t seed ) : rd_( seed ) {}

void Rand::reseed( rng::entropy const& seed ) {
  rd_.reseed( seed );
}

bool Rand::bernoulli( double p ) { return rd_.bernoulli( p ); }

int Rand::between_ints( int lower, int upper ) {
  return rd_.uniform( lower, upper );
}

double Rand::between_doubles( double lower, double upper ) {
  return rd_.uniform( lower, upper );
}

} // namespace rn
