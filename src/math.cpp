/****************************************************************
**math.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-09.
*
* Description: Various math and statistics utilities.
*
*****************************************************************/
#include "math.hpp"

// C++ standard library
#include <cmath>

using namespace std;

namespace rn {

int round_up_to_nearest_int_multiple( double d, int m ) {
  if( d < 0.0 )
    return -round_down_to_nearest_int_multiple( -d, m );
  int fl = floor( d );
  if( d == fl ) {
    // d (== fl) is integral.
    if( fl % m == 0 ) return fl;
  }
  return fl + ( m - ( fl % m ) );
}

int round_down_to_nearest_int_multiple( double d, int m ) {
  if( d < 0.0 )
    return -round_up_to_nearest_int_multiple( -d, m );
  int fl = floor( d );
  return fl - ( fl % m );
}

} // namespace rn
