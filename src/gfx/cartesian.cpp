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

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// C++ standard library
#include <cmath>

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::refl::enum_values;

template<typename R>
R rect_clamped( R const src, R const bounds ) {
  static const decltype( std::declval<R>().origin.x ) zero{};

  R res = src;
  if( bounds.size.w == 0 ) {
    res.origin.x = bounds.origin.x;
    res.size.w   = 0;
  } else {
    if( res.origin.x < bounds.origin.x ) {
      res.size.w -= ( bounds.origin.x - res.origin.x );
      res.size.w   = std::max( zero, res.size.w );
      res.origin.x = bounds.origin.x;
    }
    if( res.origin.x >= bounds.right() ) {
      res.size.w += ( res.origin.x - bounds.right() );
      res.origin.x = bounds.right();
    }
    if( res.right() > bounds.right() )
      res.size.w = bounds.right() - res.origin.x;
  }
  if( bounds.size.h == 0 ) {
    res.origin.y = bounds.origin.y;
    res.size.h   = 0;
  } else {
    if( res.origin.y < bounds.origin.y ) {
      res.size.h -= ( bounds.origin.y - res.origin.y );
      res.size.h   = std::max( zero, res.size.h );
      res.origin.y = bounds.origin.y;
    }
    if( res.origin.y >= bounds.bottom() ) {
      res.size.h += ( res.origin.y - bounds.bottom() );
      res.origin.y = bounds.bottom();
    }
    if( res.bottom() > bounds.bottom() )
      res.size.h = bounds.bottom() - res.origin.y;
  }
  return res;
}

} // namespace

/****************************************************************
** e_side
*****************************************************************/
e_side reverse( e_side const s ) {
  switch( s ) {
    case e_side::right:
      return e_side::left;
    case e_side::left:
      return e_side::right;
  }
}

/****************************************************************
** e_direction
*****************************************************************/
e_direction_type direction_type( e_direction d ) {
  switch( d ) {
    case e_direction::nw:
    case e_direction::ne:
    case e_direction::sw:
    case e_direction::se:
      return e_direction_type::diagonal;
    case e_direction::n:
    case e_direction::w:
    case e_direction::e:
    case e_direction::s:
      return e_direction_type::cardinal;
  }
}

e_direction reverse_direction( e_direction d ) {
  switch( d ) {
    case e_direction::nw:
      return e_direction::se;
    case e_direction::ne:
      return e_direction::sw;
    case e_direction::sw:
      return e_direction::ne;
    case e_direction::se:
      return e_direction::nw;
    case e_direction::n:
      return e_direction::s;
    case e_direction::w:
      return e_direction::e;
    case e_direction::e:
      return e_direction::w;
    case e_direction::s:
      return e_direction::n;
  }
}

/****************************************************************
** e_cdirection
*****************************************************************/
base::maybe<e_direction> to_direction( e_cdirection cd ) {
  base::maybe<e_direction> res;
  switch( cd ) {
    case e_cdirection::nw:
      res = e_direction::nw;
      break;
    case e_cdirection::ne:
      res = e_direction::ne;
      break;
    case e_cdirection::sw:
      res = e_direction::sw;
      break;
    case e_cdirection::se:
      res = e_direction::se;
      break;
    case e_cdirection::n:
      res = e_direction::n;
      break;
    case e_cdirection::w:
      res = e_direction::w;
      break;
    case e_cdirection::e:
      res = e_direction::e;
      break;
    case e_cdirection::s:
      res = e_direction::s;
      break;
    case e_cdirection::c:
      break;
  }
  return res;
}

e_cdirection to_cdirection( e_direction d ) {
  switch( d ) {
    case e_direction::nw:
      return e_cdirection::nw;
    case e_direction::ne:
      return e_cdirection::ne;
    case e_direction::sw:
      return e_cdirection::sw;
    case e_direction::se:
      return e_cdirection::se;
    case e_direction::n:
      return e_cdirection::n;
    case e_direction::w:
      return e_cdirection::w;
    case e_direction::e:
      return e_cdirection::e;
    case e_direction::s:
      return e_cdirection::s;
  }
}

/****************************************************************
** e_cardinal_direction
*****************************************************************/
e_direction to_direction( e_cardinal_direction d ) {
  switch( d ) {
    case e_cardinal_direction::n:
      return e_direction::n;
    case e_cardinal_direction::w:
      return e_direction::w;
    case e_cardinal_direction::e:
      return e_direction::e;
    case e_cardinal_direction::s:
      return e_direction::s;
  }
}

e_cdirection to_cdirection( e_cardinal_direction d ) {
  switch( d ) {
    case e_cardinal_direction::n:
      return e_cdirection::n;
    case e_cardinal_direction::w:
      return e_cdirection::w;
    case e_cardinal_direction::e:
      return e_cdirection::e;
    case e_cardinal_direction::s:
      return e_cdirection::s;
  }
}

/****************************************************************
** e_diagonal_direction
*****************************************************************/
e_direction to_direction( e_diagonal_direction d ) {
  switch( d ) {
    case e_diagonal_direction::nw:
      return e_direction::nw;
    case e_diagonal_direction::ne:
      return e_direction::ne;
    case e_diagonal_direction::se:
      return e_direction::se;
    case e_diagonal_direction::sw:
      return e_direction::sw;
  }
}

base::maybe<e_diagonal_direction> to_diagonal( e_direction d ) {
  switch( d ) {
    case e_direction::nw:
      return e_diagonal_direction::nw;
    case e_direction::ne:
      return e_diagonal_direction::ne;
    case e_direction::sw:
      return e_diagonal_direction::sw;
    case e_direction::se:
      return e_diagonal_direction::se;
    case e_direction::n:
    case e_direction::w:
    case e_direction::e:
    case e_direction::s:
      return base::nothing;
  }
}

e_diagonal_direction reverse_direction(
    e_diagonal_direction const d ) {
  switch( d ) {
    case e_diagonal_direction::nw:
      return e_diagonal_direction::se;
    case e_diagonal_direction::ne:
      return e_diagonal_direction::sw;
    case e_diagonal_direction::se:
      return e_diagonal_direction::nw;
    case e_diagonal_direction::sw:
      return e_diagonal_direction::ne;
  }
}

e_side side_for( e_diagonal_direction const d ) {
  switch( d ) {
    case e_diagonal_direction::nw:
      return e_side::left;
    case e_diagonal_direction::ne:
      return e_side::right;
    case e_diagonal_direction::se:
      return e_side::right;
    case e_diagonal_direction::sw:
      return e_side::left;
  }
}

/****************************************************************
** size
*****************************************************************/
size size::max_with( size const rhs ) const {
  return size{ std::max( w, rhs.w ), std::max( h, rhs.h ) };
}

size size::abs() const {
  size res = *this;
  if( res.w < 0 ) res.w = -res.w;
  if( res.h < 0 ) res.h = -res.h;
  return res;
}

double size::pythagorean() const {
  return sqrt( w * w + h * h );
}

int size::chessboard_distance() const {
  size const normalized = abs();
  return std::max( normalized.w, normalized.h );
}

size size::operator+( size const term ) const {
  return size{ .w = w + term.w, .h = h + term.h };
};

size size::operator-( size const term ) const {
  return size{ .w = w - term.w, .h = h - term.h };
};

size size::operator-() const { return size{ .w = -w, .h = -h }; }

void size::operator+=( size term ) {
  w += term.w;
  h += term.h;
}

size size::operator*( int scale ) const {
  size res = *this;
  res.w *= scale;
  res.h *= scale;
  return res;
}

size size::operator/( int scale ) const {
  size res = *this;
  res.w /= scale;
  res.h /= scale;
  return res;
}

bool size::fits_inside( size const rhs ) const {
  static auto opposite_signs = []( int const l, int const r ) {
    return ( l < 0 && r > 0 ) || ( l > 0 && r < 0 );
  };
  if( opposite_signs( w, rhs.w ) || opposite_signs( h, rhs.h ) )
    return false;
  return std::abs( w ) <= std::abs( rhs.w ) &&
         std::abs( h ) <= std::abs( rhs.h );
}

/****************************************************************
** dsize
*****************************************************************/
void dsize::operator+=( dsize term ) {
  w += term.w;
  h += term.h;
}

dsize dsize::operator*( double scale ) const {
  dsize res = *this;
  res.w *= scale;
  res.h *= scale;
  return res;
}

dsize dsize::operator/( double scale ) const {
  dsize res = *this;
  res.w /= scale;
  res.h /= scale;
  return res;
}

/****************************************************************
** point
*****************************************************************/
point point::origin() {
  static point p = {};
  return p;
}

point point::point_becomes_origin( point p ) const {
  return point{ .x = x - p.x, .y = y - p.y };
}

point point::origin_becomes_point( point p ) const {
  return point{ .x = x + p.x, .y = y + p.y };
}

point point::clamped( rect const& r ) const {
  point res = *this;
  if( y < r.origin.y ) res.y = r.origin.y;
  if( y > r.origin.y + r.size.h ) res.y = r.origin.y + r.size.h;
  if( x < r.origin.x ) res.x = r.origin.x;
  if( x > r.origin.x + r.size.w ) res.x = r.origin.x + r.size.w;
  return res;
}

bool point::is_inside( rect const& r ) const {
  return ( x >= r.origin.x ) && ( y >= r.origin.y ) &&
         ( x < r.origin.x + r.size.w ) &&
         ( y < r.origin.y + r.size.h );
}

void point::operator+=( size const s ) {
  x += s.w;
  y += s.h;
}

void point::operator-=( size const s ) {
  x -= s.w;
  y -= s.h;
}

void point::operator*=( int const scale ) {
  x *= scale;
  y *= scale;
}

void point::operator/=( int const scale ) {
  CHECK_GT( scale, 0 );
  x /= scale;
  y /= scale;
}

size point::operator-( point const rhs ) const {
  return size{ .w = x - rhs.x, .h = y - rhs.y };
}

point point::operator-( size const rhs ) const {
  return point{ .x = x - rhs.w, .y = y - rhs.h };
}

point point::operator*( int scale ) const {
  point res = *this;
  res.x *= scale;
  res.y *= scale;
  return res;
}

point point::operator/( int scale ) const {
  point res = *this;
  res.x /= scale;
  res.y /= scale;
  return res;
}

point point::operator/( size scale ) const {
  point res = *this;
  res.x /= scale.w;
  res.y /= scale.h;
  return res;
}

point point::moved_left( int by ) const {
  return point{ .x = x - by, .y = y };
}

point point::moved_right( int by ) const {
  return point{ .x = x + by, .y = y };
}

point point::moved_up( int by ) const {
  return point{ .x = x, .y = y - by };
}

point point::moved_down( int by ) const {
  return point{ .x = x, .y = y + by };
}

point point::moved( e_direction d ) const {
  // clang-format off
  switch( d ) {
    case e_direction::nw: return {.x=x-1,.y=y-1};
    case e_direction::n:  return {.x=x,  .y=y-1};
    case e_direction::ne: return {.x=x+1,.y=y-1};
    case e_direction::w:  return {.x=x-1,.y=y  };
    case e_direction::e:  return {.x=x+1,.y=y  };
    case e_direction::sw: return {.x=x-1,.y=y+1};
    case e_direction::s:  return {.x=x,  .y=y+1};
    case e_direction::se: return {.x=x+1,.y=y+1};
  };
  // clang-format on
}

point point::moved( e_cardinal_direction const d ) const {
  // clang-format off
  switch( d ) {
    case e_cardinal_direction::n:  return {.x=x,  .y=y-1};
    case e_cardinal_direction::w:  return {.x=x-1,.y=y  };
    case e_cardinal_direction::e:  return {.x=x+1,.y=y  };
    case e_cardinal_direction::s:  return {.x=x,  .y=y+1};
  };
  // clang-format on
}

point point::moved( e_diagonal_direction const d ) const {
  // clang-format off
  switch( d ) {
    case e_diagonal_direction::nw: return {.x=x-1,.y=y-1};
    case e_diagonal_direction::ne: return {.x=x+1,.y=y-1};
    case e_diagonal_direction::sw: return {.x=x-1,.y=y+1};
    case e_diagonal_direction::se: return {.x=x+1,.y=y+1};
  };
  // clang-format on
}

point point::moved( e_cdirection d ) const {
  // clang-format off
  switch( d ) {
    case e_cdirection::nw: return {.x=x-1,.y=y-1};
    case e_cdirection::n:  return {.x=x,  .y=y-1};
    case e_cdirection::ne: return {.x=x+1,.y=y-1};
    case e_cdirection::w:  return {.x=x-1,.y=y  };
    case e_cdirection::e:  return {.x=x+1,.y=y  };
    case e_cdirection::sw: return {.x=x-1,.y=y+1};
    case e_cdirection::s:  return {.x=x,  .y=y+1};
    case e_cdirection::se: return {.x=x+1,.y=y+1};
    case e_cdirection::c:  return {.x=x,  .y=y  };
  };
  // clang-format on
}

point point::with_x( int const new_x ) const {
  return { .x = new_x, .y = y };
}

point point::with_y( int const new_y ) const {
  return { .x = x, .y = new_y };
}

maybe<e_direction> point::direction_to(
    point const dest ) const {
  for( auto const d : enum_values<e_direction> )
    if( moved( d ) == dest ) //
      return d;
  return nothing;
}

/****************************************************************
** dpoint
*****************************************************************/
dsize dpoint::fmod( double d ) const {
  return dsize{ .w = std::fmod( x, d ), .h = std::fmod( y, d ) };
}

void dpoint::operator+=( dsize s ) {
  x += s.w;
  y += s.h;
}

void dpoint::operator-=( dsize s ) {
  x -= s.w;
  y -= s.h;
}

dpoint dpoint::operator-( dsize s ) const {
  return dpoint{ .x = x - s.w, .y = y - s.h };
}

dsize dpoint::operator-( dpoint rhs ) const {
  return dsize{ .w = x - rhs.x, .h = y - rhs.y };
}

dpoint dpoint::operator*( double scale ) const {
  dpoint res = *this;
  res.x *= scale;
  res.y *= scale;
  return res;
}

dpoint dpoint::operator/( double scale ) const {
  dpoint res = *this;
  res.x /= scale;
  res.y /= scale;
  return res;
}

dpoint dpoint::point_becomes_origin( dpoint p ) const {
  return dpoint{ .x = x - p.x, .y = y - p.y };
}

dpoint dpoint::origin_becomes_point( dpoint p ) const {
  return dpoint{ .x = x + p.x, .y = y + p.y };
}

dpoint dpoint::clamped( drect const& r ) const {
  dpoint res = *this;
  if( y < r.origin.y ) res.y = r.origin.y;
  if( y > r.origin.y + r.size.h ) res.y = r.origin.y + r.size.h;
  if( x < r.origin.x ) res.x = r.origin.x;
  if( x > r.origin.x + r.size.w ) res.x = r.origin.x + r.size.w;
  return res;
}

bool dpoint::is_inside( drect const& r ) const {
  return ( x >= r.origin.x ) && ( y >= r.origin.y ) &&
         ( x < r.origin.x + r.size.w ) &&
         ( y < r.origin.y + r.size.h );
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
  if( size.w >= 0 && size.h >= 0 )
    // Fast path for the (likely) usual case when the rect is al-
    // ready normalized.
    return *this;
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
  return rect{ .origin = first, .size = ( opposite - first ) }
      .normalized();
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

point rect::corner( e_diagonal_direction const d ) const {
  switch( d ) {
    case gfx::e_diagonal_direction::nw:
      return nw();
    case gfx::e_diagonal_direction::ne:
      return ne();
    case gfx::e_diagonal_direction::sw:
      return sw();
    case gfx::e_diagonal_direction::se:
      return se();
  }
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

interval rect::horizontal_slice() const {
  return interval{ .start = origin.x, .len = size.w };
}

interval rect::vertical_slice() const {
  return interval{ .start = origin.y, .len = size.h };
}

rect rect::with_new_right_edge( const int edge ) const {
  return rect{ .origin = { .x = origin.x, .y = origin.y },
               .size   = { .w = edge - origin.x, .h = size.h } }
      .normalized();
}

rect rect::with_new_left_edge( const int edge ) const {
  return rect{
    .origin = { .x = edge, .y = origin.y },
    .size = { .w = size.w + ( origin.x - edge ), .h = size.h } }
      .normalized();
}

rect rect::with_new_top_edge( const int edge ) const {
  return rect{
    .origin = { .x = origin.x, .y = edge },
    .size = { .w = size.w, .h = size.h + ( origin.y - edge ) } }
      .normalized();
}

rect rect::with_new_bottom_edge( const int edge ) const {
  return rect{ .origin = { .x = origin.x, .y = origin.y },
               .size   = { .w = size.w, .h = edge - origin.y } }
      .normalized();
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

rect rect::clamped( rect bounds ) const {
  return rect_clamped( *this, bounds );
}

rect rect::with_origin( point const p ) const {
  rect res   = *this;
  res.origin = p;
  return res;
}

rect rect::with_size( struct size const s ) const {
  rect res = *this;
  res.size = s;
  return res;
}

point rect::center() const {
  return point{
    .x = origin.x + size.w / 2,
    .y = origin.y + size.h / 2,
  };
}

rect rect::point_becomes_origin( point p ) const {
  return rect{ .origin = origin.point_becomes_origin( p ),
               .size   = size };
}

rect rect::origin_becomes_point( point p ) const {
  return rect{ .origin = origin.origin_becomes_point( p ),
               .size   = size };
}

rect rect::with_border_added( int n ) const {
  // This requires a normalized rect in order to work properly.
  CHECK_GE( size.w, 0 );
  CHECK_GE( size.h, 0 );
  return rect{
    .origin = origin.moved_up( n ).moved_left( n ),
    .size   = { .w = size.w + 2 * n, .h = size.h + 2 * n } };
}

rect rect::with_inc_size( int n ) const {
  auto r = *this;
  for( int i = 0; i < n; ++i ) {
    ++r.size.w;
    ++r.size.h;
  }
  return r;
}

rect rect::with_dec_size( int n ) const {
  auto r = *this;
  for( int i = 0; i < n; ++i ) {
    --r.size.w;
    --r.size.h;
  }
  if( r.size.w < 0 ) r.size.w = 0;
  if( r.size.h < 0 ) r.size.h = 0;
  return r;
}

rect rect::uni0n( rect const rhs ) const {
  auto const new_x1 = std::min( origin.x, rhs.origin.x );
  auto const new_y1 = std::min( origin.y, rhs.origin.y );
  auto const new_x2 =
      std::max( origin.x + size.w, rhs.origin.x + rhs.size.w );
  auto const new_y2 =
      std::max( origin.y + size.h, rhs.origin.y + rhs.size.h );
  return { .origin{ .x = new_x1, .y = new_y1 },
           .size{ .w = ( new_x2 - new_x1 ),
                  .h = ( new_y2 - new_y1 ) } };
}

rect rect::with_edges_removed( int n ) const {
  rect r( *this );

  // TODO: not very efficient.
  for( int i = 0; i < n; ++i ) {
    // We always advance location unless length is zero.
    if( size.w >= 1 ) ++r.origin.x;
    if( size.h >= 1 ) ++r.origin.y;

    r.size.w -= 2;
    r.size.h -= 2;
    if( r.size.w < 0 ) r.size.w = 0;
    if( r.size.h < 0 ) r.size.h = 0;

    if( r.size.w == 0 && r.size.h == 0 ) break;
  }

  return r;
}

rect rect::moved_left( int const by ) const {
  return rect{ .origin = origin.moved_left( by ), .size = size };
}

rect rect::moved_right( int const by ) const {
  return rect{ .origin = origin.moved_right( by ),
               .size   = size };
}

rect rect::moved_up( int const by ) const {
  return rect{ .origin = origin.moved_up( by ), .size = size };
}

rect rect::moved_down( int const by ) const {
  return rect{ .origin = origin.moved_down( by ), .size = size };
}

rect rect::moved( e_direction const d ) const {
  return rect{ .origin = origin.moved( d ), .size = size };
}

rect rect::moved( e_cardinal_direction const d ) const {
  return rect{ .origin = origin.moved( d ), .size = size };
}

rect rect::moved( e_diagonal_direction const d ) const {
  return rect{ .origin = origin.moved( d ), .size = size };
}

rect rect::moved( e_cdirection const d ) const {
  return rect{ .origin = origin.moved( d ), .size = size };
}

rect rect::operator*( int scale ) const {
  rect res   = *this;
  res.origin = res.origin * scale;
  res.size   = res.size * scale;
  return res;
}

rect rect::operator/( int scale ) const {
  rect res   = *this;
  res.origin = res.origin / scale;
  res.size   = res.size / scale;
  return res;
}

/****************************************************************
** drect
*****************************************************************/
maybe<drect> drect::clipped_by( drect const other ) const {
  drect res = this->normalized();
  if( res.right() > other.right() )
    res.size.w -= ( res.right() - other.right() );
  if( res.bottom() > other.bottom() )
    res.size.h -= ( res.bottom() - other.bottom() );
  if( res.left() < other.left() ) {
    int delta    = static_cast<int>( other.left() - res.left() );
    res.origin.x = other.left();
    res.size.w -= delta;
    if( res.right() > other.right() )
      res.size.w -= ( res.right() - other.right() );
  }
  if( res.top() < other.top() ) {
    int delta    = static_cast<int>( other.top() - res.top() );
    res.origin.y = other.top();
    res.size.h -= delta;
    if( res.bottom() > other.bottom() )
      res.size.h -= ( res.bottom() - other.bottom() );
  }
  if( res.size.negative() ) return nothing;
  CHECK_GE( res.top(), other.top() );
  CHECK_GE( res.left(), other.left() );
  CHECK_LE( res.bottom(), other.bottom() );
  CHECK_LE( res.right(), other.right() );
  // Note that res.size.area() could be zero here.
  return res;
}

dpoint drect::nw() const { return normalized().origin; }

dpoint drect::ne() const {
  drect norm = normalized();
  return dpoint{ .x = norm.origin.x + norm.size.w,
                 .y = norm.origin.y };
}

dpoint drect::se() const {
  drect norm = normalized();
  return norm.origin + norm.size;
}

dpoint drect::sw() const {
  drect norm = normalized();
  return dpoint{ .x = norm.origin.x,
                 .y = norm.origin.y + norm.size.h };
}

double drect::top() const {
  drect norm = normalized();
  return norm.origin.y;
}

double drect::bottom() const {
  drect norm = normalized();
  return norm.origin.y + norm.size.h;
}

double drect::left() const {
  drect norm = normalized();
  return norm.origin.x;
}

double drect::right() const {
  drect norm = normalized();
  return norm.origin.x + norm.size.w;
}

drect drect::normalized() const {
  drect res = *this;
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

rect drect::truncated() const {
  return rect{ .origin = origin.truncated(),
               .size   = size.truncated() };
}

drect drect::clamped( drect bounds ) const {
  return rect_clamped( *this, bounds );
}

drect drect::point_becomes_origin( dpoint p ) const {
  return drect{ .origin = origin.point_becomes_origin( p ),
                .size   = size };
}

drect drect::origin_becomes_point( dpoint p ) const {
  return drect{ .origin = origin.origin_becomes_point( p ),
                .size   = size };
}

drect drect::operator*( double scale ) const {
  drect res  = *this;
  res.origin = res.origin * scale;
  res.size   = res.size * scale;
  return res;
}

drect drect::operator/( double scale ) const {
  drect res  = *this;
  res.origin = res.origin / scale;
  res.size   = res.size / scale;
  return res;
}

/****************************************************************
** Free Functions
*****************************************************************/
dpoint centered_in( dsize s, drect r ) {
  return { .x = r.origin.x + r.size.w / 2 - s.w / 2,
           .y = r.origin.y + r.size.h / 2 - s.h / 2 };
}

point centered_in( size s, rect r ) {
  return { .x = r.origin.x + r.size.w / 2 - s.w / 2,
           .y = r.origin.y + r.size.h / 2 - s.h / 2 };
}

point centered_at( size const s, rect const r,
                   e_cdirection const d ) {
  switch( d ) {
    case e_cdirection::c:
      return centered_in( s, r );
    case e_cdirection::e:
      return centered_at_right( s, r );
    case e_cdirection::n:
      return centered_at_top( s, r );
    case e_cdirection::ne:
      return { .x = centered_at_right( s, r ).x,
               .y = centered_at_top( s, r ).y };
    case e_cdirection::nw:
      return { .x = centered_at_left( s, r ).x,
               .y = centered_at_top( s, r ).y };
    case e_cdirection::s:
      return centered_at_bottom( s, r );
    case e_cdirection::se:
      return { .x = centered_at_right( s, r ).x,
               .y = centered_at_bottom( s, r ).y };
    case e_cdirection::sw:
      return { .x = centered_at_left( s, r ).x,
               .y = centered_at_bottom( s, r ).y };
    case e_cdirection::w:
      return centered_at_left( s, r );
  }
}

point centered_at_bottom( size const s, rect const r ) {
  point upper_left = centered_in( s, r );
  upper_left.y     = r.bottom() - s.h;
  return upper_left;
}

point centered_at_top( size const s, rect const r ) {
  point upper_left = centered_in( s, r );
  upper_left.y     = r.origin.y;
  return upper_left;
}

point centered_at_left( size const s, rect const r ) {
  point upper_left = centered_in( s, r );
  upper_left.x     = r.origin.x;
  return upper_left;
}

point centered_at_right( size const s, rect const r ) {
  point upper_left = centered_in( s, r );
  upper_left.x     = r.right() - s.w;
  return upper_left;
}

rect centered_on( size const s, point const p ) {
  return rect{
    .origin = { .x = p.x - s.w / 2, .y = p.y - s.h / 2 },
    .size   = s };
}

/****************************************************************
** oriented_point.
*****************************************************************/
oriented_point oriented_point::point_becomes_origin(
    point const p ) const {
  return oriented_point{
    .anchor    = anchor.point_becomes_origin( p ),
    .placement = placement };
}

oriented_point oriented_point::origin_becomes_point(
    point const p ) const {
  return oriented_point{
    .anchor    = anchor.origin_becomes_point( p ),
    .placement = placement };
}

point find_placement( oriented_point const& op, size const sz ) {
  point res = op.anchor;
  switch( op.placement ) {
    case e_cdirection::c:
      res -= sz / 2;
      break;
    case e_cdirection::e:
      res.x -= sz.w;
      res.y -= sz.h / 2;
      break;
    case e_cdirection::n:
      res.x -= sz.w / 2;
      break;
    case e_cdirection::ne:
      res.x -= sz.w;
      break;
    case e_cdirection::nw:
      break;
    case e_cdirection::s:
      res.x -= sz.w / 2;
      res.y -= sz.h;
      break;
    case e_cdirection::se:
      res.x -= sz.w;
      res.y -= sz.h;
      break;
    case e_cdirection::sw:
      res.y -= sz.h;
      break;
    case e_cdirection::w:
      res.y -= sz.h / 2;
      break;
  }
  return res;
}

/****************************************************************
** Combining Operators
*****************************************************************/
point operator+( point const p, size const s ) {
  return point{ .x = p.x + s.w, .y = p.y + s.h };
}

point operator+( size const s, point const p ) { return p + s; }

dpoint operator+( dpoint const p, dsize const s ) {
  return dpoint{ .x = p.x + s.w, .y = p.y + s.h };
}

dpoint operator+( dsize const s, dpoint const p ) {
  return p + s;
}

point operator*( point const p, size const s ) {
  return point{ .x = p.x * s.w, .y = p.y * s.h };
}

} // namespace gfx
