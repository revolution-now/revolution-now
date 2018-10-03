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

#include "macros.hpp"

#include <exception>
#include <stdexcept>
#include <iostream>
#include <utility>

using namespace std;

namespace rn {

namespace {


  
} // namespace

Rect Rect::from( Coord const& _1, Coord const& _2 ) {
  Rect res;
  res.x = min( _1.x, _2.x );
  res.y = min( _1.y, _2.y );
  res.w = max( _1.x, _2.x ) - min( _1.x, _2.x );
  res.h = max( _1.y, _2.y ) - min( _1.y, _2.y );
  return res;
}

Rect from( Coord const& coord, Delta const& delta ) {
  return {coord.x,coord.y,delta.w,delta.h};
}

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

Rect Rect::uni0n( Rect const& rhs ) const {
  auto new_x1 = min( x, rhs.x );
  auto new_y1 = min( y, rhs.y );
  auto new_x2 = max( x+w, rhs.x+rhs.w );
  auto new_y2 = max( y+h, rhs.y+rhs.h );
  return {new_x1, new_y1, (new_x2-new_x1), (new_y2-new_y1)};
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
  DIE( "should not get here." );
  return {y,x}; // to silence warning; will not get here.
}

bool Coord::is_inside( Rect const& rect ) {
  return (x >= rect.x)        &&
         (y >= rect.y)        &&
         (x <  rect.x+rect.w) &&
         (y <  rect.y+rect.h);
}

Delta Delta::uni0n( Delta const& rhs ) const {
  return {max( w, rhs.w ), max( h, rhs.h ) };
}

Coord operator+( Coord const& coord, Delta const& delta ) {
  return {coord.y+delta.h, coord.x+delta.w};
}

Coord operator+( Delta const& delta, Coord const& coord ) {
  return {coord.y+delta.h, coord.x+delta.w};
}

Delta operator-( Coord const& lhs, Coord const& rhs ) {
  return {lhs.x-rhs.x, lhs.y-rhs.y};
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

std::ostream& operator<<( std::ostream& out, rn::Rect const& r ) {
  return (out << "(" << r.x << "," << r.y << "," << r.w << "," << r.h << ")");
}

std::ostream& operator<<( std::ostream& out, rn::Coord const& coord ) {
  return (out << "(" << coord.x << "," << coord.y << ")");
}

