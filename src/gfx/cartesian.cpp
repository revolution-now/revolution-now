/****************************************************************
**cartesian.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-26.
*
* Description: Structs for working in cartesian space.
*
*****************************************************************/
#include "cartesian.hpp"

using namespace std;

namespace gfx {

/****************************************************************
** size
*****************************************************************/
size size::max_with( size const& rhs ) const {
  return size{ std::max( w, rhs.w ), std::max( h, rhs.h ) };
}

/****************************************************************
** point
*****************************************************************/
point const& point::origin() {
  static point p = {};
  return p;
}

/****************************************************************
** rect
*****************************************************************/
bool rect::contains( point const& p ) const {
  return ( p.x >= origin.x ) && ( p.y >= origin.y ) &&
         ( se().x >= p.x ) && ( se().y >= p.y );
}

bool rect::is_inside( rect const& other ) const {
  rect l = normalized();
  rect r = other.normalized();
  return ( l.origin.x >= r.origin.x ) &&
         ( l.origin.y >= r.origin.y ) &&
         ( l.origin.x + l.size.w <= r.origin.x + r.size.w ) &&
         ( l.origin.y + l.size.h <= r.origin.y + r.size.h );
}

rect rect::normalized() const {
  rect res = *this;
  if( res.size.w < 0 ) {
    res.origin.x += res.size.w;
    res.size.w = -res.size.w;
  }
  if( res.size.h < 0 ) {
    res.origin.y += res.size.h;
    res.size.h = -res.size.h;
  }
  return res;
}

point rect::nw() const { return normalized().origin; }

point rect::ne() const {
  rect norm = normalized();
  return point{ .x = norm.origin.x + norm.size.w,
                .y = norm.origin.y };
}

point rect::se() const {
  rect norm = normalized();
  return norm.origin + norm.size;
}

point rect::sw() const {
  rect norm = normalized();
  return point{ .x = norm.origin.x,
                .y = norm.origin.y + norm.size.h };
}

/****************************************************************
** Combining Operators
*****************************************************************/
point operator+( point const& p, size const& s ) {
  return point{ .x = p.x + s.w, .y = p.y + s.h };
}

point operator+( size const& s, point const& p ) {
  return p + s;
}
} // namespace gfx
