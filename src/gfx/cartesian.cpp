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

using ::base::maybe;
using ::base::nothing;

/****************************************************************
** size
*****************************************************************/
size size::max_with( size const rhs ) const {
  return size{ std::max( w, rhs.w ), std::max( h, rhs.h ) };
}

size size::operator*( int factor ) const {
  size res = *this;
  res.w *= factor;
  res.h *= factor;
  return res;
}

size size::operator/( int factor ) const {
  size res = *this;
  res.w /= factor;
  res.h /= factor;
  return res;
}

void size::operator+=( size term ) {
  w += term.w;
  h += term.h;
}

/****************************************************************
** point
*****************************************************************/
point point::origin() {
  static point p = {};
  return p;
}

point point::moved_left( int by ) const {
  return point{ .x = x - by, .y = y };
}

/****************************************************************
** rect
*****************************************************************/
bool rect::contains( point const p ) const {
  return ( p.x >= origin.x ) && ( p.y >= origin.y ) &&
         ( se().x >= p.x ) && ( se().y >= p.y );
}

bool rect::is_inside( rect const other ) const {
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

rect rect::from( point first, point opposite ) {
  return rect{ .origin = first, .size = ( opposite - first ) };
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

int rect::top() const {
  rect norm = normalized();
  return norm.origin.y;
}

int rect::bottom() const {
  rect norm = normalized();
  return norm.origin.y + norm.size.h;
}

int rect::left() const {
  rect norm = normalized();
  return norm.origin.x;
}

int rect::right() const {
  rect norm = normalized();
  return norm.origin.x + norm.size.w;
}

maybe<rect> rect::clipped_by( rect const other ) const {
  rect res = this->normalized();
  if( res.right() > other.right() )
    res.size.w -= ( res.right() - other.right() );
  if( res.bottom() > other.bottom() )
    res.size.h -= ( res.bottom() - other.bottom() );
  if( res.left() < other.left() ) {
    int delta = ( other.left() - res.left() );
    res.origin.x += delta;
    res.size.w -= delta;
  }
  if( res.top() < other.top() ) {
    int delta = ( other.top() - res.top() );
    res.origin.y += delta;
    res.size.h -= delta;
  }
  if( res.size.negative() ) return nothing;
  // Note that res.size.area() could be zero here.
  return res;
}

rect rect::with_origin( point const p ) const {
  rect res   = *this;
  res.origin = p;
  return res;
}

point rect::center() const {
  return point{
      .x = origin.x + size.w / 2,
      .y = origin.y + size.h / 2,
  };
}

/****************************************************************
** Combining Operators
*****************************************************************/
point operator+( point const p, size const s ) {
  return point{ .x = p.x + s.w, .y = p.y + s.h };
}

point operator+( size const s, point const p ) { return p + s; }

size operator+( size const s1, size const s2 ) {
  return size{ .w = s1.w + s2.w, .h = s1.h + s2.h };
};

void operator+=( point& p, size const s ) { p = p + s; }

point operator*( point const p, size const s ) {
  return point{ .x = p.x * s.w, .y = p.y * s.h };
}

size operator-( point const p1, point const p2 ) {
  return size{ .w = p1.x - p2.x, .h = p1.y - p2.y };
}

} // namespace gfx
