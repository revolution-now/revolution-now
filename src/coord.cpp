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

// Revolution Now
#include "error.hpp"
#include "lua.hpp"

// luapp
#include "luapp/as.hpp"
#include "luapp/func-push.hpp"
#include "luapp/metatable.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// base
#include "base/fmt.hpp"

// c++ standard library
#include <algorithm>
#include <cmath>
#include <utility>

using namespace std;

namespace rn {

namespace {} // namespace

void to_str( Scale const& o, string& out, base::ADL_t ) {
  out += "(";
  to_str( o.sx, out, base::ADL );
  out += ",";
  to_str( o.sy, out, base::ADL );
  out += ")";
}

void to_str( Rect const& o, std::string& out, base::ADL_t ) {
  out += "(";
  to_str( o.x, out, base::ADL );
  out += ",";
  to_str( o.y, out, base::ADL );
  out += ",";
  to_str( o.w, out, base::ADL );
  out += ",";
  to_str( o.h, out, base::ADL );
  out += ")";
}

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
Rect Rect::edges_removed() const {
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

Rect Rect::with_border_added( int thickness ) const {
  W wd{ thickness };
  H hd{ thickness };
  return { x - wd, y - hd, w + wd + wd, h + hd + hd };
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
  return { new_x1, new_y1, ( new_x2 - new_x1 ),
           ( new_y2 - new_y1 ) };
}

bool Rect::is_inside( Rect const& rect ) const {
  // lower_right() is one-past-the-end, so we need to bump it
  // back by one.
  return this->upper_left().is_inside( rect ) &&
         ( this->lower_right() - Delta{ 1_w, 1_h } )
             .is_inside( rect );
}

maybe<Rect> Rect::overlap_with( Rect const& rhs ) const {
  // NOTE: be careful here with returning references; we should
  // only be using auto const& when the function will not return
  // a reference to a temporary.
  auto const& new_x1 = std::max( x, rhs.x );
  auto const& new_y1 = std::max( y, rhs.y );
  auto /*!!*/ new_x2 = std::min( x + w, rhs.x + rhs.w );
  auto /*!!*/ new_y2 = std::min( y + h, rhs.y + rhs.h );
  maybe<Rect> res    = Rect::from( Coord{ new_x1, new_y1 },
                                   Coord{ new_x2, new_y2 } );
  if( res->area() == 0 ) res = nothing;
  return res;
}

Rect Rect::clamp( Rect const& rect ) const {
  Rect res = *this;
  if( rect.w == 0_w ) {
    res.x = rect.x;
    res.w = 0;
  } else {
    if( res.x < rect.x ) res.x = rect.x;
    if( res.x >= rect.right_edge() ) res.x = rect.right_edge();
    if( res.x + res.w > rect.right_edge() )
      res.w = rect.right_edge() - res.x;
  }
  if( rect.h == 0_h ) {
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
  return Rect::from( coord - this->delta() / Scale{ 2 },
                     this->delta() );
}

maybe<int> Rect::rasterize( Coord coord ) {
  if( !coord.is_inside( *this ) ) return nothing;
  return ( coord.y - y )._ * w._ + ( coord.x - x )._;
}

RectGridProxyIteratorHelper Rect::to_grid_noalign(
    Delta delta ) const& {
  return RectGridProxyIteratorHelper( *this, delta );
}

int RectGridProxyIteratorHelper::const_iterator::operator-(
    RectGridProxyIteratorHelper::const_iterator const& rhs )
    const {
  auto delta = it - rhs.it;
  return delta.h._ * rect_proxy->rect.w._ + delta.w._;
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

void to_str( Coord const& o, std::string& out, base::ADL_t ) {
  out += "(";
  to_str( o.x, out, base::ADL );
  out += ",";
  to_str( o.y, out, base::ADL );
  out += ")";
}

template<>
X const& Coord::coordinate<X>() const {
  return x;
}
template<>
Y const& Coord::coordinate<Y>() const {
  return y;
}

void Coord::clip( Rect const& rect ) {
  if( y < rect.y ) y = rect.y;
  if( y > rect.y + rect.h ) y = rect.y + rect.h;
  if( x < rect.x ) x = rect.x;
  if( x > rect.x + rect.w ) x = rect.x + rect.w;
}

Coord Coord::rounded_to_multiple_to_plus_inf(
    Delta multiple ) const {
  CHECK( multiple.w > 0_w );
  CHECK( multiple.h > 0_h );
  auto res     = *this;
  auto mod     = res % multiple;
  auto delta_w = res.x >= 0_x ? ( multiple.w - mod.w ) : -mod.w;
  auto delta_h = res.y >= 0_y ? ( multiple.h - mod.h ) : -mod.h;
  CHECK( delta_w >= 0_w );
  CHECK( delta_h >= 0_h );
  // These must be done separately.
  if( mod.w != 0_w ) res.x += delta_w;
  if( mod.h != 0_h ) res.y += delta_h;
  return res;
}

Coord Coord::rounded_to_multiple_to_plus_inf(
    Scale multiple ) const {
  return rounded_to_multiple_to_plus_inf( Delta{ 1_w, 1_h } *
                                          multiple );
}

Coord Coord::rounded_to_multiple_to_minus_inf(
    Delta multiple ) const {
  CHECK( multiple.w > 0_w );
  CHECK( multiple.h > 0_h );
  auto res     = *this;
  auto mod     = res % multiple;
  auto delta_w = res.x >= 0_x ? mod.w : ( multiple.w + mod.w );
  auto delta_h = res.y >= 0_y ? mod.h : ( multiple.h + mod.h );
  CHECK( delta_w >= 0_w );
  CHECK( delta_h >= 0_h );
  // These must be done separately.
  if( mod.w != 0_w ) res.x -= delta_w;
  if( mod.h != 0_h ) res.y -= delta_h;
  return res;
}

Coord Coord::rounded_to_multiple_to_minus_inf(
    Scale multiple ) const {
  return rounded_to_multiple_to_minus_inf( Delta{ 1_w, 1_h } *
                                           multiple );
}

Coord Coord::moved( e_direction d ) const {
  // clang-format off
  switch( d ) {
    case e_direction::nw: return {y-1,x-1}; break;
    case e_direction::n:  return {y-1,x  }; break;
    case e_direction::ne: return {y-1,x+1}; break;
    case e_direction::w:  return {y,  x-1}; break;
    case e_direction::c:  return {y,  x  }; break;
    case e_direction::e:  return {y,  x+1}; break;
    case e_direction::sw: return {y+1,x-1}; break;
    case e_direction::s:  return {y+1,x  }; break;
    case e_direction::se: return {y+1,x+1}; break;
  };
  // clang-format on
  SHOULD_NOT_BE_HERE;
}

maybe<e_direction> Coord::direction_to( Coord dest ) const {
  for( auto d : enum_traits<e_direction>::values )
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
  auto direction = direction_to( other );
  if( direction.has_value() &&
      direction.value() != e_direction::c )
    return true;
  return false;
}

bool Coord::is_inside( Rect const& rect ) const {
  return ( x >= rect.x ) && ( y >= rect.y ) &&
         ( x < rect.x + rect.w ) && ( y < rect.y + rect.h );
}

bool Coord::is_on_border_of( Rect const& rect ) const {
  return is_inside( rect ) && !is_inside( rect.edges_removed() );
}

void to_str( Delta const& o, std::string& out, base::ADL_t ) {
  out += "(";
  to_str( o.w, out, base::ADL );
  out += ",";
  to_str( o.h, out, base::ADL );
  out += ")";
}

Delta Delta::round_up( Scale grid_size ) const {
  Coord bottom_right = Coord{} + *this;
  return bottom_right
      .rounded_to_multiple_to_plus_inf( grid_size )
      .distance_from_origin();
}

Delta Delta::trimmed_by_one() const {
  auto res = *this;
  if( res.w > 0 )
    res.w -= 1_w;
  else if( res.w < 0 )
    res.w += 1_w;
  if( res.h > 0 )
    res.h -= 1_h;
  else if( res.h < 0 )
    res.h += 1_h;
  return res;
}

double Delta::diagonal() const {
  return std::sqrt( w._ * w._ + h._ * h._ );
}

Delta Delta::projected_along( Delta const& along ) const {
  double mag = along.diagonal();
  DCHECK( mag >= 0 );
  // If we effectively don't have a projection direction than the
  // resultant vector will be of zero length, which means that
  // the new end == start.
  if( mag == 0 ) return {};
  auto dot_product = inner_product( *this, along );
  return along.multiply_and_round( dot_product / ( mag * mag ) );
}

Delta Delta::multiply_and_round( double scale ) const {
  return Delta{
      W{ static_cast<int>( std::lround( w._ * scale ) ) },
      H{ static_cast<int>( std::lround( h._ * scale ) ) } };
}

Delta Delta::uni0n( Delta const& rhs ) const {
  return { std::max( w, rhs.w ), std::max( h, rhs.h ) };
}

Delta Delta::clamp( Delta const& delta ) const {
  return { std::min( w, delta.w ), std::min( h, delta.h ) };
}

RectGridProxyIteratorHelper::const_iterator begin(
    RectGridProxyIteratorHelper const& rect_proxy ) {
  return rect_proxy.begin();
}
RectGridProxyIteratorHelper::const_iterator end(
    RectGridProxyIteratorHelper const& rect_proxy ) {
  return rect_proxy.end();
}

Coord centered( Delta const& delta, Rect const& rect ) {
  return { rect.y + rect.h / 2 - delta.h / 2,
           rect.x + rect.w / 2 - delta.w / 2 };
}

Coord centered_bottom( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.y     = rect.bottom_edge() - delta.h;
  return upper_left;
}

Coord centered_top( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.y     = 0_y;
  return upper_left;
}

Coord centered_left( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.x     = 0_x;
  return upper_left;
}

Coord centered_right( Delta const& delta, Rect const& rect ) {
  Coord upper_left = centered( delta, rect );
  upper_left.x     = rect.right_edge() - delta.w;
  return upper_left;
}

int inner_product( Delta const& fst, Delta const& snd ) {
  return fst.w._ * snd.w._ + fst.h._ * snd.h._;
}

Delta max( Delta const& lhs, Delta const& rhs ) {
  return { std::max( lhs.w, rhs.w ), std::max( lhs.h, rhs.h ) };
}

Delta min( Delta const& lhs, Delta const& rhs ) {
  return { std::min( lhs.w, rhs.w ), std::min( lhs.h, rhs.h ) };
}

Coord operator+( Coord const& coord, Delta const& delta ) {
  return { coord.y + delta.h, coord.x + delta.w };
}

Delta operator-( Delta const& lhs, Delta const& rhs ) {
  return { lhs.w - rhs.w, lhs.h - rhs.h };
}

Coord operator+( Delta const& delta, Coord const& coord ) {
  return { coord.y + delta.h, coord.x + delta.w };
}

Coord operator-( Coord const& coord, Delta const& delta ) {
  return { coord.y - delta.h, coord.x - delta.w };
}

Rect operator+( Rect const& rect, Delta const& delta ) {
  return { rect.x + delta.w, rect.y + delta.h, rect.w, rect.h };
}

Rect operator+( Delta const& delta, Rect const& rect ) {
  return { rect.x + delta.w, rect.y + delta.h, rect.w, rect.h };
}

Rect operator-( Rect const& rect, Delta const& delta ) {
  return { rect.x - delta.w, rect.y - delta.h, rect.w, rect.h };
}

ND Coord operator+( Coord const& coord, W w ) {
  return { coord.y, coord.x + w };
}

ND Coord operator+( Coord const& coord, H h ) {
  return { coord.y + h, coord.x };
}

ND Coord operator-( Coord const& coord, W w ) {
  return { coord.y, coord.x - w };
}

ND Coord operator-( Coord const& coord, H h ) {
  return { coord.y - h, coord.x };
}

void operator+=( Coord& coord, W w ) { coord.x += w; }
void operator+=( Coord& coord, H h ) { coord.y += h; }
void operator-=( Coord& coord, W w ) { coord.x -= w; }
void operator-=( Coord& coord, H h ) { coord.y -= h; }
void operator-=( Coord& coord, Delta delta ) {
  coord -= delta.w;
  coord -= delta.h;
}

ND Delta operator+( Delta const& delta, W w ) {
  return { delta.h, delta.w + w };
}

ND Delta operator+( Delta const& delta, H h ) {
  return { delta.h + h, delta.w };
}

ND Delta operator-( Delta const& delta, W w ) {
  return { delta.h, delta.w - w };
}

ND Delta operator-( Delta const& delta, H h ) {
  return { delta.h - h, delta.w };
}

void operator+=( Delta& delta, W w ) { delta.w += w; }
void operator+=( Delta& delta, H h ) { delta.h += h; }
void operator-=( Delta& delta, W w ) { delta.w -= w; }
void operator-=( Delta& delta, H h ) { delta.h -= h; }

Delta operator-( Coord const& lhs, Coord const& rhs ) {
  return { lhs.x - rhs.x, lhs.y - rhs.y };
}

ND Coord operator*( Coord const& coord, Scale const& scale ) {
  Coord res = coord;
  res *= scale;
  return res;
}

ND Coord operator*( Scale const& scale, Coord const& coord ) {
  Coord res = coord;
  res *= scale;
  return res;
}

ND Delta operator*( Scale const& scale, Delta const& delta ) {
  Delta res = delta;
  res *= scale;
  return res;
}

Rect operator*( Rect const& rect, Scale const& scale ) {
  return Rect::from( rect.upper_left() * scale,
                     rect.delta() * scale );
}

Rect operator*( Scale const& scale, Rect const& rect ) {
  return Rect::from( rect.upper_left() * scale,
                     rect.delta() * scale );
}

Coord operator/( Coord const& coord, Scale const& scale ) {
  return Coord{ coord.x / scale.sx, coord.y / scale.sy };
}

Delta operator/( Delta const& delta, Scale const& scale ) {
  return Delta{ delta.w / scale.sx, delta.h / scale.sy };
}

Rect operator/( Rect const& rect, Scale const& scale ) {
  auto coord = rect.upper_left();
  auto delta = Delta{ rect.w, rect.h };
  return Rect::from( coord / scale, delta / scale );
}

Delta operator%( Coord const& coord, Scale const& scale ) {
  return { coord.x % scale.sx, coord.y % scale.sy };
}

Delta operator%( Coord const& coord, Delta const& delta ) {
  return { coord.x % delta.w, coord.y % delta.h };
}

Scale operator*( Scale const& lhs, Scale const& rhs ) {
  return { lhs.sx * rhs.sx, lhs.sy * rhs.sy };
}

Scale operator/( Scale const& lhs, Scale const& rhs ) {
  return { lhs.sx / rhs.sx, lhs.sy / rhs.sy };
}

/****************************************************************
** Rcl
*****************************************************************/
rcl::convert_err<Delta> convert_to( rcl::value const& v,
                                    rcl::tag<Delta> ) {
  constexpr string_view kTypeName          = "Delta";
  constexpr int         kNumFieldsExpected = 2;
  base::maybe<std::unique_ptr<rcl::table> const&> mtbl =
      v.get_if<std::unique_ptr<rcl::table>>();
  if( !mtbl )
    return rcl::error(
        fmt::format( "cannot produce a Delta from type {}.",
                     rcl::name_of( rcl::type_of( v ) ) ) );
  DCHECK( *mtbl != nullptr );
  rcl::table const& tbl = **mtbl;
  if( !tbl.has_key( "w" ) || !tbl.has_key( "h" ) ||
      tbl.size() != kNumFieldsExpected )
    return rcl::error(
        fmt::format( "table must have a 'w' and 'h' field for "
                     "conversion to {}.",
                     kTypeName ) );
  UNWRAP_RETURN( w, rcl::convert_to<W>( tbl["w"] ) );
  UNWRAP_RETURN( h, rcl::convert_to<H>( tbl["h"] ) );
  return Delta{ w, h };
}

rcl::convert_err<Coord> convert_to( rcl::value const& v,
                                    rcl::tag<Coord> ) {
  constexpr string_view kTypeName          = "Coord";
  constexpr int         kNumFieldsExpected = 2;
  base::maybe<std::unique_ptr<rcl::table> const&> mtbl =
      v.get_if<std::unique_ptr<rcl::table>>();
  if( !mtbl )
    return rcl::error(
        fmt::format( "cannot produce a Coord from type {}.",
                     rcl::name_of( rcl::type_of( v ) ) ) );
  DCHECK( *mtbl != nullptr );
  rcl::table const& tbl = **mtbl;
  if( !tbl.has_key( "x" ) || !tbl.has_key( "y" ) ||
      tbl.size() != kNumFieldsExpected )
    return rcl::error(
        fmt::format( "table must have a 'x' and 'y' field for "
                     "conversion to {}.",
                     kTypeName ) );
  UNWRAP_RETURN( x, rcl::convert_to<X>( tbl["x"] ) );
  UNWRAP_RETURN( y, rcl::convert_to<Y>( tbl["y"] ) );
  return Coord{ x, y };
}

/****************************************************************
** Lua
*****************************************************************/
maybe<Coord> lua_get( lua::cthread L, int idx,
                      lua::tag<Coord> ) {
  auto st = lua::state::view( L );

  maybe<lua::table> maybe_t = lua::get<lua::table>( L, idx );
  if( !maybe_t.has_value() ) return nothing;
  lua::table& t = *maybe_t;
  if( t["x"] == lua::nil || t["y"] == lua::nil ) return nothing;
  Coord coord{ lua::as<X>( t["x"] ), lua::as<Y>( t["y"] ) };
  return coord;
}

void lua_push( lua::cthread L, Coord const& coord ) {
  auto st = lua::state::view( L );

  lua::table t = st.table.create();
  t["x"]       = coord.x;
  t["y"]       = coord.y;

  // Delegate to the Lua factory function because it puts some
  // metatables in there for us.
  lua::push( L, st["Coord"]( t ) );
}

LUA_ENUM( direction );

} // namespace rn
