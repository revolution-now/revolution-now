/****************************************************************
**geo-types.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-08.
*
* Description: Holds types related to world geography.
*
*****************************************************************/
#include "geo-types.hpp"

// Revolution Now
#include "errors.hpp"

// c++ standard library
#include <algorithm>
#include <iostream>
#include <utility>

using namespace std;

namespace rn {

namespace {} // namespace

Rect Rect::from( Coord const& _1, Coord const& _2 ) {
  Rect res;
  res.x = std::min( _1.x, _2.x );
  res.y = std::min( _1.y, _2.y );
  res.w = std::max( _1.x, _2.x ) - std::min( _1.x, _2.x );
  res.h = std::max( _1.y, _2.y ) - std::min( _1.y, _2.y );
  return res;
}

Rect Rect::from( Coord const& coord, Delta const& delta ) {
  return {coord.x, coord.y, delta.w, delta.h};
}

// New coord equal to this one unit of edge trimmed off
// on all sides.  (width,height) ==> (width-2,height-2)
Rect Rect::edges_removed() {
  Rect rect( *this );

  // We always advance location unless length is zero.
  if( w >= 1_w ) ++rect.x;
  if( h >= 1_h ) ++rect.y;

  rect.w -= 2;
  rect.h -= 2;
  if( rect.w < 0 ) rect.w = 0;
  if( rect.h < 0 ) rect.h = 0;

  return rect;
}

Rect Rect::uni0n( Rect const& rhs ) const {
  // NOTE: be careful here with returning references; we should
  // only be using auto const& when the function will not return
  // a reference to a temporary.
  auto const& new_x1 = std::min( x, rhs.x );
  auto const& new_y1 = std::min( y, rhs.y );
  auto /*!!*/ new_x2 = std::max( x + w, rhs.x + rhs.w );
  auto /*!!*/ new_y2 = std::max( y + h, rhs.y + rhs.h );
  return {new_x1, new_y1, ( new_x2 - new_x1 ),
          ( new_y2 - new_y1 )};
}

template<>
X const& Rect::coordinate<X>() const {
  return x;
}
template<>
Y const& Rect::coordinate<Y>() const {
  return y;
}
template<>
W const& Rect::length<X>() const {
  return w;
}
template<>
H const& Rect::length<Y>() const {
  return h;
}

template<>
X const& Coord::coordinate<X>() const {
  return x;
}
template<>
Y const& Coord::coordinate<Y>() const {
  return y;
}

Coord Coord::moved( direction d ) {
  // clang-format off
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
  // clang-format on
  DIE( "should not get here." );
  return {y, x}; // to silence warning; will not get here.
}

bool Coord::is_inside( Rect const& rect ) const {
  return ( x >= rect.x ) && ( y >= rect.y ) &&
         ( x < rect.x + rect.w ) && ( y < rect.y + rect.h );
}

Delta Delta::uni0n( Delta const& rhs ) const {
  return {std::max( w, rhs.w ), std::max( h, rhs.h )};
}

Delta max( Delta const& lhs, Delta const& rhs ) {
  return {std::max( lhs.w, rhs.w ), std::max( lhs.h, rhs.h )};
}

ND Delta operator*( int scale, Delta const& delta ) {
  return {delta.w * scale, delta.h * scale};
}

Coord operator+( Coord const& coord, Delta const& delta ) {
  return {coord.y + delta.h, coord.x + delta.w};
}

Delta operator-( Delta const& lhs, Delta const& rhs ) {
  return {lhs.w - rhs.w, lhs.h - rhs.h};
}

Coord operator+( Delta const& delta, Coord const& coord ) {
  return {coord.y + delta.h, coord.x + delta.w};
}

ND Coord operator+( Coord const& coord, W w ) {
  return {coord.y, coord.x + w};
}

ND Coord operator+( Coord const& coord, H h ) {
  return {coord.y + h, coord.x};
}

Delta operator-( Coord const& lhs, Coord const& rhs ) {
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}

std::ostream& operator<<( std::ostream&   out,
                          rn::Rect const& r ) {
  return ( out << "(" << r.x << "," << r.y << "," << r.w << ","
               << r.h << ")" );
}

std::ostream& operator<<( std::ostream&    out,
                          rn::Coord const& coord ) {
  return ( out << "(" << coord.x << "," << coord.y << ")" );
}

} // namespace rn
