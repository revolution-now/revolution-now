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
void Rand::reseed( rng::entropy const& seed ) {
  rd_.reseed( seed );
}

bool Rand::bernoulli( double p ) { return rd_.bernoulli( p ); }

int Rand::uniform_int( int lower, int upper ) {
  return rd_.uniform_int( lower, upper );
}

double Rand::uniform_double( double lower, double upper ) {
  return rd_.uniform_double( lower, upper );
}

rng::seed Rand::generate_deterministic_seed() {
  return rd_.generate_deterministic_seed();
}

} // namespace rn
