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

double Rand::normal( double const mean, double const stddev ) {
  return rd_.normal( mean, stddev );
}

double Rand::piecewise3( double const peak,
                         double const weight_at_0,
                         double const weight_at_1 ) {
  static auto const C = []( double const d ) {
    return clamp( d, 0.0, 1.0 );
  };

  return rd_.piecewise( rng::random::piecewise3{
    .l = { .value = 0.0, .weight = C( weight_at_0 ) },
    .m = { .value = C( peak ), .weight = 1.0 },
    .r = { .value = 1.0, .weight = C( weight_at_1 ) },
  } );
}

rng::seed Rand::generate_deterministic_seed() {
  return rd_.generate_deterministic_seed();
}

} // namespace rn
