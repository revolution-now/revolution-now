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
bool random::bernoulli( double p ) {
  CHECK_GE( p, 0 );
  CHECK_LE( p, 1.0 );
  return bernoulli_distribution( p )( engine_ );
}

int random::uniform( int lower, int upper ) {
  CHECK_LE( lower, upper );
  return uniform_int_distribution<int>( lower,
                                        upper )( engine_ );
}

double random::uniform( double lower, double upper ) {
  CHECK_LE( lower, upper );
  return uniform_real_distribution<double>( lower,
                                            upper )( engine_ );
}

} // namespace base
