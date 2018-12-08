/****************************************************************
**util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description:
*
*****************************************************************/
#include "util.hpp"

#include "errors.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace std;

namespace rn {

namespace {} // namespace

int round_up_to_nearest_int_multiple( double d, int m ) {
  int floor = int( d );
  if( floor % m != 0 ) floor += m;
  return floor / m;
}

int round_down_to_nearest_int_multiple( double d, int m ) {
  int floor = int( d );
  return floor / m;
}

} // namespace rn
