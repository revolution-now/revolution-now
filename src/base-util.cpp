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

#include <exception>
#include <stdexcept>
#include <iostream>
#include <utility>

using namespace std;

namespace rn {

namespace {


  
} // namespace

// New coord equal to this one unit of edge trimmed off
// on all sides.  (width,height) ==> (width-2,height-2)
Rect Rect::edges_removed() {
  Rect rect( *this );

  // We always advance location unless length is zero.
  if( w >= W(1) ) ++rect.x;
  if( h >= H(1) ) ++rect.y;

  rect.w -= 2; rect.h -= 2;
  if( rect.w < 0 ) rect.w = 0;
  if( rect.h < 0 ) rect.h = 0;

  return rect;
}

template<> X& Rect::coordinate<X>() { return x; }
template<> Y& Rect::coordinate<Y>() { return y; }
template<> W& Rect::length<X>() { return w; }
template<> H& Rect::length<Y>() { return h; }

template<> X& Coord::coordinate<X>() { return x; }
template<> Y& Coord::coordinate<Y>() { return y; }

Coord Coord::moved( direction d ) {
  switch( d ) {
    case direction::nw: return {y-1,x-1}; break;
    case direction::n:  return {y-1,x  }; break;
    case direction::ne: return {y-1,x+1}; break;
    case direction::w:  return {y,  x-1}; break;
    case direction::c:  return {y,  x  }; break;
    case direction::e:  return {y,  x+1}; break;
    case direction::sw: return {y+1,x-1}; break;
    case direction::s:  return {y+1,x  }; break;
    case direction::se: return {y+1,x+1}; break;
  };
}

bool Coord::is_inside( Rect const& rect ) {
  return (x >= rect.x)        &&
         (y >= rect.y)        &&
         (x <  rect.x+rect.w) &&
         (y <  rect.y+rect.h);
}

std::ostream& operator<<( std::ostream& out, Rect const& r ) {
  return (out << "(" << r.x << "," << r.y << "," << r.w << "," << r.h << ")");
}

void die( char const* file, int line, std::string_view msg ) {
  std::cerr << "error:" << file << ":" << line << ": " << msg << "\n";
  //std::terminate();
  throw logic_error( "terminating program. see log for error" );
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

