/****************************************************************
* base-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: 
*
*****************************************************************/

#include "base-util.hpp"

#include <iostream>
#include <utility>

namespace rn {

namespace {


  
} // namespace

std::ostream& operator<<( std::ostream& out, Rect const& r ) {
  return (out << "(" << r.x << "," << r.y << "," << r.w << "," << r.h << ")");
}

void die( char const* file, int line, std::string_view msg ) {
  std::cerr << "error:" << file << ":" << line << ": " << msg << "\n";
  std::terminate();
}

int round_up_to_nearest_int_multiple( double d, int m ) {
  int floor = int( d );
  if( floor % m != 0 )
    floor += m;
  return floor/m;
}

int round_down_to_nearest_int_multiple( double d, int m ) {
  int floor = int( d );
  return floor/m;
}

} // namespace rn

