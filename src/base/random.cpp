/****************************************************************
**random.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Basic randomness facilities.
*
*****************************************************************/
#include "random.hpp"

using namespace std;

namespace base {

/****************************************************************
** random
*****************************************************************/
void random::reseed( uint32_t const new_seed ) {
  engine_ = engine_t( new_seed );
}

bool random::bernoulli( double const p ) {
  CHECK_GE( p, 0 );
  CHECK_LE( p, 1.0 );
  return bernoulli_distribution( p )( engine_ );
}

int random::uniform( int const lower, int const upper ) {
  CHECK_LE( lower, upper );
  return uniform_int_distribution<int>( lower,
                                        upper )( engine_ );
}

double random::uniform( double const lower,
                        double const upper ) {
  CHECK_LE( lower, upper );
  return uniform_real_distribution<double>( lower,
                                            upper )( engine_ );
}

} // namespace base
