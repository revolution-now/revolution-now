/****************************************************************
**coord.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-08.
*
* Description: Holds types related to abstract coordinates.
*
*****************************************************************/
#include "coord.hpp"

// luapp
#include "luapp/as.hpp"
#include "luapp/func-push.hpp"
#include "luapp/metatable.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/fmt.hpp"

// c++ standard library
#include <algorithm>
#include <cmath>
#include <utility>

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** e_direction
*****************************************************************/
e_direction_type direction_type( e_direction d ) {
  switch( d ) {
    case e_direction::nw:
    case e_direction::ne:
    case e_direction::sw:
    case e_direction::se: return e_direction_type::diagonal;
    case e_direction::n:
    case e_direction::w:
    case e_direction::e:
    case e_direction::s: return e_direction_type::cardinal;
  }
}

/****************************************************************
** Rect
*****************************************************************/
Rect Rect::from( Coord const& _1, Coord const& _2 ) {
  Rect res;
  res.x = std::min( _1.x, _2.x );
  res.y = std::min( _1.y, _2.y );
  res.w = std::max( _1.x, _2.x ) - std::min( _1.x, _2.x );
  res.h = std::max( _1.y, _2.y ) - std::min( _1.y, _2.y );
  return res;
}

Rect Rect::from( Coord const& coord, Delta const& delta ) {
  return from( coord, coord + delta );
}

// New coord equal to this one unit of edge trimmed off
// on all sides.  (width,height) ==> (width-2,height-2)
Rect Rect::edges_removed( int times ) const {
  Rect rect( *this );

  for( int i = 0; i < times; ++i ) {
    // We always advance location unless length is zero.
    if( w >= 1 ) ++rect.x;
    if( h >= 1 ) ++rect.y;

    rect.w -= 2;
    rect.h -= 2;
    if( rect.w < 0 ) rect.w = 0;
    if( rect.h < 0 ) rect.h = 0;
  }

  return rect;
}

Rect Rect::with_border_added( int thickness ) const {
  W wd{ thickness };
  H hd{ thickness };
  return { .x = x - wd,
           .y = y - hd,
           .w = w + wd + wd,
           .h = h + hd + hd };
}

Rect Rect::with_new_origin( Coord new_origin ) const {
  return Rect::from(
      Coord{ .x = x, .y = y }.with_new_origin( new_origin ),
      Delta{ .w = w, .h = h } );
}

Rect Rect::as_if_origin_were( Coord const& coord ) const {
  return this->shifted_by( coord.distance_from_origin() );
}

Rect Rect::normalized() const {
  Rect res = *this;
  if( res.w < 0 ) {
    res.x += res.w;
    res.w = -res.w;
  }
  if( res.h < 0 ) {
    res.y += res.h;
    res.h = -res.h;
  }
  return res;
}

Rect Rect::uni0n( Rect const& rhs ) const {
  // NOTE: be careful here with returning references; we should
  // only be using auto const& when the function will not return
  // a reference to a temporary.
  auto const& new_x1 = std::min( x, rhs.x );
  auto const& new_y1 = std::min( y, rhs.y );
  auto /*!!*/ new_x2 = std::max( x + w, rhs.x + rhs.w );
  auto /*!!*/ new_y2 = std::max( y + h, rhs.y + rhs.h );
  return { .x = new_x1,
           .y = new_y1,
           .w = ( new_x2 - new_x1 ),
           .h = ( new_y2 - new_y1 ) };
}

bool Rect::is_inside( Rect const& rect ) const {
  // lower_right() is one-past-the-end, so we need to bump it
  // back by one.
  return this->upper_left().is_inside( rect ) &&
         ( this->lower_right() - Delta{ .w = 1, .h = 1 } )
             .is_inside( rect );
}

base::maybe<Rect> Rect::overlap_with( Rect const& rhs ) const {
  // NOTE: be careful here with returning references; we should
  // only be using auto const& when the function will not return
  // a reference to a temporary.
  auto const&       new_x1 = std::max( x, rhs.x );
  auto const&       new_y1 = std::max( y, rhs.y );
  auto /*!!*/       new_x2 = std::min( x + w, rhs.x + rhs.w );
  auto /*!!*/       new_y2 = std::min( y + h, rhs.y + rhs.h );
  base::maybe<Rect> res =
      Rect::from( Coord{ .x = new_x1, .y = new_y1 },
                  Coord{ .x = new_x2, .y = new_y2 } );
  if( res->area() == 0 ) res = base::nothing;
  return res;
}

Rect Rect::clamp( Rect const& rect ) const {
  Rect res = *this;
  if( rect.w == 0 ) {
    res.x = rect.x;
    res.w = 0;
  } else {
    if( res.x < rect.x ) res.x = rect.x;
    if( res.x >= rect.right_edge() ) res.x = rect.right_edge();
    if( res.x + res.w > rect.right_edge() )
      res.w = rect.right_edge() - res.x;
  }
  if( rect.h == 0 ) {
    res.y = rect.y;
    res.h = 0;
  } else {
    if( res.y < rect.y ) res.y = rect.y;
    if( res.y >= rect.bottom_edge() ) res.y = rect.bottom_edge();
    if( res.y + res.h > rect.bottom_edge() )
      res.h = rect.bottom_edge() - res.y;
  }
  return res;
}

Rect Rect::centered_on( Coord coord ) const {
  return Rect::from( coord - this->delta() / 2, this->delta() );
}

base::maybe<int> Rect::rasterize( Coord coord ) const {
  if( !coord.is_inside( *this ) ) return base::nothing;
  return ( coord.y - y ) * w + ( coord.x - x );
}

template<>
int Rect::coordinate<DimensionX>() const {
  return x;
}
template<>
int Rect::coordinate<DimensionY>() const {
  return y;
}
template<>
int Rect::length<DimensionX>() const {
  return w;
}
template<>
int Rect::length<DimensionY>() const {
  return h;
}

template<>
int Coord::coordinate<DimensionX>() const {
  return x;
}
template<>
int Coord::coordinate<DimensionY>() const {
  return y;
}

void Coord::clamp( Rect const& rect ) {
  if( y < rect.y ) y = rect.y;
  if( y > rect.y + rect.h ) y = rect.y + rect.h;
  if( x < rect.x ) x = rect.x;
  if( x > rect.x + rect.w ) x = rect.x + rect.w;
}

Coord Coord::rounded_to_multiple_to_plus_inf(
    Delta multiple ) const {
  CHECK( multiple.w > 0 );
  CHECK( multiple.h > 0 );
  auto res     = *this;
  auto mod     = res % multiple;
  auto delta_w = res.x >= 0 ? ( multiple.w - mod.w ) : -mod.w;
  auto delta_h = res.y >= 0 ? ( multiple.h - mod.h ) : -mod.h;
  CHECK( delta_w >= 0 );
  CHECK( delta_h >= 0 );
  // These must be done separately.
  if( mod.w != 0 ) res.x += delta_w;
  if( mod.h != 0 ) res.y += delta_h;
  return res;
}

Coord Coord::rounded_to_multiple_to_minus_inf(
    Delta multiple ) const {
  CHECK( multiple.w > 0 );
  CHECK( multiple.h > 0 );
  auto res     = *this;
  auto mod     = res % multiple;
  auto delta_w = res.x >= 0 ? mod.w : ( multiple.w + mod.w );
  auto delta_h = res.y >= 0 ? mod.h : ( multiple.h + mod.h );
  CHECK( delta_w >= 0 );
  CHECK( delta_h >= 0 );
  // These must be done separately.
  if( mod.w != 0 ) res.x -= delta_w;
  if( mod.h != 0 ) res.y -= delta_h;
  return res;
}

Coord Coord::moved( e_direction d ) const {
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

base::maybe<e_direction> Coord::direction_to(
    Coord dest ) const {
  for( auto d : refl::enum_values<e_direction> )
    if( moved( d ) == dest ) return d;
  return {};
}

Coord Coord::with_new_origin( Coord new_origin ) const {
  return Coord{} + ( ( *this ) - new_origin );
}

Coord Coord::as_if_origin_were( Coord const& coord ) const {
  return ( *this ) + ( coord - Coord{} );
}

bool Coord::is_adjacent_to( Coord other ) const {
  return direction_to( other ).has_value();
}

bool Coord::is_inside( Rect const& rect ) const {
  return ( x >= rect.x ) && ( y >= rect.y ) &&
         ( x < rect.x + rect.w ) && ( y < rect.y + rect.h );
}

bool Coord::is_on_border_of( Rect const& rect ) const {
  return is_inside( rect ) && !is_inside( rect.edges_removed() );
}

Delta Delta::round_up( Delta grid_size ) const {
  Coord bottom_right = Coord{} + *this;
  return bottom_right
      .rounded_to_multiple_to_plus_inf( grid_size )
      .distance_from_origin();
}

Delta Delta::trimmed_by_one() const {
  auto res = *this;
  if( res.w > 0 )
    res.w -= 1;
  else if( res.w < 0 )
    res.w += 1;
  if( res.h > 0 )
    res.h -= 1;
  else if( res.h < 0 )
    res.h += 1;
  return res;
}

double Delta::diagonal() const {
  return std::sqrt( w * w + h * h );
}

Delta Delta::multiply_and_round( double scale ) const {
  return Delta{
      W{ static_cast<int>( std::lround( w * scale ) ) },
      H{ static_cast<int>( std::lround( h * scale ) ) } };
}

Delta Delta::multiply_and_truncate( double scale ) const {
  return Delta{ W{ static_cast<int>( w * scale ) },
                H{ static_cast<int>( h * scale ) } };
}

Delta Delta::uni0n( Delta const& rhs ) const {
  return { .w = std::max( w, rhs.w ),
           .h = std::max( h, rhs.h ) };
}

Delta Delta::clamp( Delta const& delta ) const {
  return { .w = std::min( w, delta.w ),
           .h = std::min( h, delta.h ) };
}

Coord centered( Delta const& delta, Rect const& rect ) {
  return { .x = rect.x + rect.w / 2 - delta.w / 2,
           .y = rect.y + rect.h / 2 - delta.h / 2 };
}

Coord centered_bottom( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.y     = rect.bottom_edge() - delta.h;
  return upper_left;
}

Coord centered_top( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.y     = 0;
  return upper_left;
}

Coord centered_left( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.x     = 0;
  return upper_left;
}

Coord centered_right( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.x     = rect.right_edge() - delta.w;
  return upper_left;
}

int inner_product( Delta const& fst, Delta const& snd ) {
  return fst.w * snd.w + fst.h * snd.h;
}

Delta max( Delta const& lhs, Delta const& rhs ) {
  return { .w = std::max( lhs.w, rhs.w ),
           .h = std::max( lhs.h, rhs.h ) };
}

Delta min( Delta const& lhs, Delta const& rhs ) {
  return { .w = std::min( lhs.w, rhs.w ),
           .h = std::min( lhs.h, rhs.h ) };
}

Coord operator+( Coord const& coord, Delta const& delta ) {
  return { .x = coord.x + delta.w, .y = coord.y + delta.h };
}

Delta operator-( Delta const& lhs, Delta const& rhs ) {
  return { .w = lhs.w - rhs.w, .h = lhs.h - rhs.h };
}

Coord operator+( Delta const& delta, Coord const& coord ) {
  return { .x = coord.x + delta.w, .y = coord.y + delta.h };
}

Coord operator-( Coord const& coord, Delta const& delta ) {
  return { .x = coord.x - delta.w, .y = coord.y - delta.h };
}

Rect operator+( Rect const& rect, Delta const& delta ) {
  return { .x = rect.x + delta.w,
           .y = rect.y + delta.h,
           .w = rect.w,
           .h = rect.h };
}

Rect operator+( Delta const& delta, Rect const& rect ) {
  return { .x = rect.x + delta.w,
           .y = rect.y + delta.h,
           .w = rect.w,
           .h = rect.h };
}

Rect operator-( Rect const& rect, Delta const& delta ) {
  return { .x = rect.x - delta.w,
           .y = rect.y - delta.h,
           .w = rect.w,
           .h = rect.h };
}

void operator-=( Coord& coord, Delta delta ) {
  coord.x -= delta.w;
  coord.y -= delta.h;
}

Delta operator-( Coord const& lhs, Coord const& rhs ) {
  return { .w = lhs.x - rhs.x, .h = lhs.y - rhs.y };
}

ND Coord operator*( Coord const& coord, Delta const& delta ) {
  Coord res = coord;
  res *= delta;
  return res;
}

ND Coord operator*( Coord const& coord, int scale ) {
  Coord res = coord;
  res.x *= scale;
  res.y *= scale;
  return res;
}

ND Coord operator*( Delta const& delta, Coord const& coord ) {
  Coord res = coord;
  res *= delta;
  return res;
}

ND Delta operator*( Delta const& lhs, Delta const& rhs ) {
  Delta res = lhs;
  res.w *= rhs.w;
  res.h *= rhs.h;
  return res;
}

Rect operator*( Rect const& rect, Delta const& delta ) {
  return Rect::from( rect.upper_left() * delta,
                     rect.delta() * delta );
}

Rect operator*( Rect const& rect, int scale ) {
  return Rect::from( rect.upper_left() * scale,
                     rect.delta() * scale );
}

Rect operator*( Delta const& delta, Rect const& rect ) {
  return Rect::from( rect.upper_left() * delta,
                     rect.delta() * delta );
}

Coord operator/( Coord const& coord, Delta const& delta ) {
  return Coord{ .x = ( coord.x / delta.w ),
                .y = ( coord.y / delta.h ) };
}

Delta operator/( Delta const& delta, int scale ) {
  return Delta{ .w = ( delta.w / scale ),
                .h = ( delta.h / scale ) };
}

Rect operator/( Rect const& rect, Delta const& delta ) {
  auto coord     = rect.upper_left();
  auto our_delta = Delta{ rect.w, rect.h };
  return Rect::from( coord / delta, our_delta / delta );
}

Delta operator%( Coord const& coord, Delta const& delta ) {
  return { .w = ( coord.x % delta.w ),
           .h = ( coord.y % delta.h ) };
}

Delta operator/( Delta const& lhs, Delta const& rhs ) {
  return { .w = ( lhs.w / rhs.w ), .h = ( lhs.h / rhs.h ) };
}

/****************************************************************
** Lua
*****************************************************************/
base::maybe<Coord> lua_get( lua::cthread L, int idx,
                            lua::tag<Coord> ) {
  auto st = lua::state::view( L );

  base::maybe<lua::table> maybe_t =
      lua::get<lua::table>( L, idx );
  if( !maybe_t.has_value() ) return base::nothing;
  lua::table& t = *maybe_t;
  if( t["x"] == lua::nil || t["y"] == lua::nil )
    return base::nothing;
  Coord coord{ lua::as<X>( t["x"] ), lua::as<Y>( t["y"] ) };
  return coord;
}

void lua_push( lua::cthread L, Coord const& coord ) {
  auto st = lua::state::view( L );

  lua::table t = st.table.create();
  t["x"]       = coord.x;
  t["y"]       = coord.y;

  lua::push( L, t );
}

base::maybe<Delta> lua_get( lua::cthread L, int idx,
                            lua::tag<Delta> ) {
  auto st = lua::state::view( L );

  base::maybe<lua::table> maybe_t =
      lua::get<lua::table>( L, idx );
  if( !maybe_t.has_value() ) return base::nothing;
  lua::table& t = *maybe_t;
  if( t["w"] == lua::nil || t["h"] == lua::nil )
    return base::nothing;
  Delta delta{ lua::as<W>( t["w"] ), lua::as<H>( t["h"] ) };
  return delta;
}

void lua_push( lua::cthread L, Delta const& delta ) {
  auto st = lua::state::view( L );

  lua::table t = st.table.create();
  t["w"]       = delta.w;
  t["h"]       = delta.h;

  lua::push( L, t );
}

} // namespace rn
