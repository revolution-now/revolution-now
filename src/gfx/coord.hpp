/****************************************************************
**coord.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-08.
*
* Description: Holds types related to abstract coordinates.
*
*****************************************************************/
#pragma once

// gfx
#include "cartesian.hpp"

// Rds
#include "coord.rds.hpp"

// luapp
#include "luapp/ext.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/maybe.hpp"

// c++ standard library
#include <cmath>

namespace rn {

using X  = int;
using Y  = int;
using W  = int;
using H  = int;
using SX = int;
using SY = int;

struct DimensionX;
struct DimensionY;
struct DimensionW;
struct DimensionH;

/****************************************************************
** Fwd Decls
*****************************************************************/
struct Coord;
struct Delta;
struct Rect;

/****************************************************************
** e_direction
*****************************************************************/
e_direction_type direction_type( e_direction d );

e_direction reverse_direction( e_direction d );

/****************************************************************
** e_cardinal_direction
*****************************************************************/
e_direction to_direction( e_cardinal_direction d );

/****************************************************************
** e_diagonal_direction
*****************************************************************/
e_direction to_direction( e_diagonal_direction d );

base::maybe<e_diagonal_direction> to_diagonal( e_direction d );

/****************************************************************
** Delta
*****************************************************************/
struct Delta {
  W w = 0;
  H h = 0;

  operator gfx::size() const {
    return gfx::size{ .w = w, .h = h };
  }

  gfx::size to_gfx() const { return *this; }

  static Delta from_gfx( gfx::size s ) {
    return Delta{ .w = s.w, .h = s.h };
  }

  constexpr bool operator==( Delta const& other ) const {
    return ( h == other.h ) && ( w == other.w );
  }

  constexpr bool operator!=( Delta const& other ) const {
    return ( h != other.h ) || ( w != other.w );
  }

  template<typename Dimension>
  auto get() const {
    if constexpr( std::is_same_v<Dimension, DimensionX> )
      return w;
    if constexpr( std::is_same_v<Dimension, DimensionY> )
      return h;
    if constexpr( std::is_same_v<Dimension, DimensionW> )
      return w;
    if constexpr( std::is_same_v<Dimension, DimensionH> )
      return h;
  }

  Delta operator-() { return { -w, -h }; }

  void operator+=( Delta const& other ) {
    w += other.w;
    h += other.h;
  }
  void operator-=( Delta const& other ) {
    w -= other.w;
    h -= other.h;
  }

  static Delta const& zero() {
    static Delta const zero{};
    return zero;
  }

  Delta with_height( H h_ ) const {
    return Delta{ .w = w, .h = h_ };
  }
  Delta with_width( W w_ ) const {
    return Delta{ .w = w_, .h = h };
  }

  // Given a grid size this will round each dimension up to the
  // nearest multiple of that size.
  Delta round_up( Delta grid_size ) const;

  // This will reduce the magnitude of each component by one.
  // E.g., if the h component is -5, it will become -4, if the w
  // component is 10, it will become 9. If a component is zero it
  // will remain zero.
  Delta trimmed_by_one() const;

  // Returns a new Delta with each component made non-negative.
  [[nodiscard]] Delta abs() const;

  // Returns the length of the diagonal according to Pythagoras.
  double diagonal() const;

  Delta mirrored_vertically() const {
    return { .w = w, .h = -h };
  }
  Delta mirrored_horizontally() const {
    return { .w = -w, .h = h };
  }

  // Multiply both components by the scale and round.
  Delta multiply_and_round( double scale ) const;

  // Multiply both components by the scale and truncate.
  Delta multiply_and_truncate( double scale ) const;

  // Result will be the smallest delta that encompasses both
  // this one and the parameter.
  Delta uni0n( Delta const& rhs ) const;

  // Will clamp each dimension individually to be within the
  // bounds of the given delta.
  [[nodiscard]] Delta clamp( Delta const& delta ) const;

  int area() const { return w * h; }

  friend base::maybe<Delta> lua_get( lua::cthread L, int idx,
                                     lua::tag<Delta> );
  friend void lua_push( lua::cthread L, Delta const& delta );
};

/****************************************************************
** Coord
*****************************************************************/
struct Coord {
  X x = 0;
  Y y = 0;

  operator gfx::point() const {
    return gfx::point{ .x = x, .y = y };
  }

  gfx::point to_gfx() const { return *this; }

  static Coord from_gfx( gfx::point p ) {
    return Coord{ .x = p.x, .y = p.y };
  }

  Coord with_x( X new_x ) const {
    return { .x = new_x, .y = y };
  }

  Coord with_y( Y new_y ) const {
    return { .x = x, .y = new_y };
  }

  // Useful for generic code; allows referencing a coordinate
  // from the type.
  template<typename Dimension>
  int coordinate() const;

  bool operator==( Coord const& other ) const {
    return ( y == other.y ) && ( x == other.x );
  }

  bool operator!=( Coord const& other ) const {
    return !( *this == other );
  }

  void operator+=( Delta const& delta ) {
    x += delta.w;
    y += delta.h;
  }

  void operator*=( Delta const& delta ) {
    x *= delta.w;
    y *= delta.h;
  }

  void operator/=( Delta const& delta ) {
    x /= delta.w;
    y /= delta.h;
  }

  Coord operator-() const { return { -x, -y }; }

  // If this coord is outside the rect then it will be brought
  // into the rect by traveling in precisely one straight line
  // in each direction (or possibly only one direction).
  void clamp( Rect const& rect );

  Coord rounded_to_multiple_to_plus_inf( Delta multiple ) const;
  Coord rounded_to_multiple_to_minus_inf( Delta multiple ) const;

  Delta distance_from_origin() const {
    return { .w = x, .h = y };
  }

  Coord moved( e_direction d ) const;
  // Find the direction from this coord to `dest`. If dest is not
  // equal or adjacent to this coord then nothing will be re-
  // turned.
  base::maybe<e_direction> direction_to( Coord dest ) const;

  // How many concentric squares around this square do we have to
  // draw before we arrive at a square that includes `other`.
  int concentric_square_distance( Coord other ) const;

  // Returns this coordinate with respect to a new origin.
  Coord with_new_origin( Coord new_origin ) const;

  // If current origin were coord instead of 0,0.
  Coord as_if_origin_were( Coord const& coord ) const;

  bool is_adjacent_to( Coord other ) const;

  bool is_inside( Rect const& rect ) const;

  // True if e.g. x is in [x,x+w).
  bool is_on_border_of( Rect const& rect ) const;

  friend base::maybe<Coord> lua_get( lua::cthread L, int idx,
                                     lua::tag<Coord> );
  friend void lua_push( lua::cthread L, Coord const& coord );
};

// This will be a lexical ordering (y, then x) so that we can use
// Coords in contexts where they need some well-defined ordering.
inline auto operator<=>( Coord l, Coord r ) {
  if( l.y != r.y ) return l.y <=> r.y;
  return l.x <=> r.x;
}

/****************************************************************
** Rect
*****************************************************************/
struct Rect {
  X x = 0;
  Y y = 0;
  W w = 0;
  H h = 0;

  static Rect from( Coord const& _1, Coord const& _2 );
  static Rect from( Coord const& coord, Delta const& delta );

  operator gfx::rect() const {
    return gfx::rect{ .origin = { .x = x, .y = y },
                      .size   = { .w = w, .h = h } };
  }

  gfx::rect to_gfx() const { return *this; }

  static Rect from_gfx( gfx::rect r ) {
    return Rect::from( Coord{ .x = r.origin.x, .y = r.origin.y },
                       Delta{ .w = r.size.w, .h = r.size.h } );
  }

  bool operator==( Rect const& rhs ) const {
    return ( x == rhs.x ) && ( y == rhs.y ) && ( w == rhs.w ) &&
           ( h == rhs.h );
  }

  bool operator!=( Rect const& rhs ) const {
    return !( *this == rhs );
  }

  // Useful for generic code; allows referencing a coordinate
  // from the type.
  template<typename Dimension>
  int coordinate() const;

  // Useful for generic code; allows referencing a width/height
  // from the of the associated dimension, i.e., with Dimension=X
  // it will return the width of type (W).
  template<typename Dimension>
  int length() const;

  // Upper left corner as a coordinate.
  Coord upper_left() const { return Coord{ .x = x, .y = y }; }
  // Lower right corner; NOTE, this is one-past-the-end.
  Coord lower_right() const {
    return Coord{ .x = x + w, .y = y + h };
  }
  // Lower left corner; NOTE, this is one-past-the-end.
  Coord lower_left() const {
    return Coord{ .x = x, .y = y + h };
  }
  // Upper right corner; NOTE, this is one-past-the-end.
  Coord upper_right() const {
    return Coord{ .x = x + w, .y = y };
  }
  // Center
  Coord center() const {
    return Coord{ .x = x + w / 2, .y = y + h / 2 };
  }

  [[nodiscard]] Rect centered_on( Coord coord ) const;

  Delta delta() const { return { w, h }; }

  // Right edge; NOTE: this is one-past-the-end.
  X right_edge() const { return x + w; }
  // Left edge
  X left_edge() const { return x; }
  // Right edge; NOTE: this is one-past-the-end.
  Y bottom_edge() const { return y + h; }
  // Left edge
  Y top_edge() const { return y; }

  // Is the rect inside another rect. "inside" means that it can
  // be fully inside, or its borders may be overlapping.
  bool is_inside( Rect const& rect ) const;

  // Returns true if any part of the two rects overlap, which
  // does not include the borders touching.
  base::maybe<Rect> overlap_with( Rect const& rhs ) const;

  // Returns a rect that is adjusted (without respecting
  // proportions) so that it fits inside the given rect.
  [[nodiscard]] Rect clamp( Rect const& rect ) const;

  // New coord equal to this one unit of edge trimmed off
  // on all sides.  That is, we will have:
  //
  //   (width,height) ==> (width-2,height-2)
  //
  // unless one of the dimensions is initially 1 or 0 in
  // which case that dimension will be 0 in the result.
  //
  // For the (x,y) coordinates we will always have:
  //
  //   (x,y) ==> (x+1,y+1)
  //
  // unless one of the dimensions has width 0 in which case
  // that dimension will remain as-is.
  [[nodiscard]] Rect edges_removed( int times = 1 ) const;

  [[nodiscard]] Rect with_border_added(
      int thickness = 1 ) const;

  [[nodiscard]] Rect shifted_by( Delta const& delta ) const {
    return Rect{ x + delta.w, y + delta.h, w, h };
  }
  [[nodiscard]] Rect with_new_upper_left(
      Coord const& coord ) const {
    return Rect{ coord.x, coord.y, w, h };
  }

  // Returns this rect with respect to a new origin.
  [[nodiscard]] Rect with_new_origin( Coord new_origin ) const;

  [[nodiscard]] Rect as_if_origin_were(
      Coord const& coord ) const;

  [[nodiscard]] Rect with_inc_size() const {
    return { x, y, w + 1, h + 1 };
  }

  [[nodiscard]] Rect with_new_right_edge( X xp ) const {
    return Rect{ x, y, xp - x, h }.normalized();
  }
  [[nodiscard]] Rect with_new_left_edge( X xp ) const {
    return Rect{ xp, y, w + ( x - xp ), h }.normalized();
  }
  [[nodiscard]] Rect with_new_top_edge( Y yp ) const {
    return Rect{ x, yp, w, h + ( y - yp ) }.normalized();
  }
  [[nodiscard]] Rect with_new_bottom_edge( Y yp ) const {
    return Rect{ x, y, w, yp - y }.normalized();
  }

  // Returns a rect that is equivalent to this one but where x
  // and y represent the upper left corner of the rect.
  [[nodiscard]] Rect normalized() const;

  // Result will be the smallest rect that encompasses both
  // this one and the parameter.
  [[nodiscard]] Rect uni0n( Rect const& rhs ) const;

  // Will return y*w + x if the coord is in the rect.
  base::maybe<int> rasterize( Coord coord ) const;

  int area() const { return delta().area(); }
};

/****************************************************************
** Misc Functions
*****************************************************************/
// Will take the delta and center it with respect to the rect and
// return the coordinate of the upper-left corner of the centered
// rect.  Note that the coord returned may be negative.
Coord centered( Delta const& delta, Rect const& rect );

// Will return the upper-left coordinate of a rect obtained by
// centering the delta along the given "wall" of the rect.
Coord centered_bottom( Delta const& delta, Rect const& rect );
Coord centered_top( Delta const& delta, Rect const& rect );
Coord centered_left( Delta const& delta, Rect const& rect );
Coord centered_right( Delta const& delta, Rect const& rect );

// Takes the "dot product".
int inner_product( Delta const& fst, Delta const& snd );

// Same as Delta::uni0n
ND Delta max( Delta const& lhs, Delta const& rhs );

ND Delta min( Delta const& lhs, Delta const& rhs );

/****************************************************************
** Algebra
*****************************************************************/
ND Delta operator-( Delta const& lhs, Delta const& rhs );
ND inline constexpr Delta operator+( Delta const& lhs,
                                     Delta const& rhs ) {
  return { lhs.w + rhs.w, lhs.h + rhs.h };
}

ND Coord operator+( Coord const& coord, Delta const& delta );
ND Coord operator+( Delta const& delta, Coord const& coord );
ND Coord operator-( Coord const& coord, Delta const& delta );
ND Delta operator-( Coord const& lhs, Coord const& rhs );

// Adding a Delta to a Rect will shift the position of the Rect.
ND Rect operator+( Rect const& rect, Delta const& delta );
ND Rect operator+( Delta const& delta, Rect const& rect );
ND Rect operator-( Rect const& rect, Delta const& delta );

void operator-=( Coord& coord, Delta delta );

ND Coord operator*( Coord const& coord, Delta const& delta );
ND Coord operator*( Coord const& coord, int scale );
ND inline constexpr Delta operator*( Delta const& delta,
                                     int          scale ) {
  Delta res = delta;
  res.w *= scale;
  res.h *= scale;
  return res;
}
ND Coord operator*( Delta const& delta, Coord const& coord );
ND Delta operator*( Delta const& lhs, Delta const& rhs );
ND Rect  operator*( Rect const& rect, Delta const& delta );
ND Rect  operator*( Rect const& rect, int scale );
ND Rect  operator*( Delta const& delta, Rect const& rect );
// FIXME: deprecated
ND Rect  operator/( Rect const& rect, Delta const& delta );
ND Coord operator/( Coord const& coord, Delta const& delta );
ND Delta operator/( Delta const& delta, int scale );
ND Delta operator/( Delta const& lhs, Delta const& rhs );
ND Delta operator%( Coord const& coord, Delta const& delta );
ND constexpr Delta operator%( Delta const& lhs,
                              Delta const& rhs ) {
  return Delta{ lhs.w % rhs.w, lhs.h % rhs.h };
}

Delta operator*( Delta const& lhs, Delta const& rhs );
Delta operator/( Delta const& lhs, Delta const& rhs );

} // namespace rn

/****************************************************************
** std::hash
*****************************************************************/
// Here we open up the std namespace to add a hash function spe-
// cialization for a Coord.
namespace std {
template<>
struct hash<::rn::Coord> {
  auto operator()( ::rn::Coord const& c ) const noexcept {
    // Required by std::bit_cast.
    static_assert( sizeof( c.x ) == sizeof( uint32_t ) );
    static_assert( sizeof( c.y ) == sizeof( uint32_t ) );
    // This assumes that the coordinate's components will be less
    // than 2^32. If that is violated, then it may not be a good
    // hash function. Also, this should support negative coordi-
    // nates as well.
    uint64_t const flat =
        ( static_cast<uint64_t>( std::bit_cast<uint32_t>( c.y ) )
          << 32 ) +
        static_cast<uint64_t>( std::bit_cast<uint32_t>( c.x ) );
    return hash<uint64_t>{}( flat );
  }
};

} // namespace std

/****************************************************************
** Reflection
*****************************************************************/
namespace refl {

// Reflection info for struct Delta.
template<>
struct traits<rn::Delta> {
  using type = rn::Delta;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "rn";
  static constexpr std::string_view name = "Delta";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "w", &rn::Delta::w,
                         offsetof( type, w ) },
      refl::StructField{ "h", &rn::Delta::h,
                         offsetof( type, h ) },
  };
};

// Reflection info for struct Coord.
template<>
struct traits<rn::Coord> {
  using type = rn::Coord;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "rn";
  static constexpr std::string_view name = "Coord";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &rn::Coord::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &rn::Coord::y,
                         offsetof( type, y ) },
  };
};

// Reflection info for struct Rect.
template<>
struct traits<rn::Rect> {
  using type = rn::Rect;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "rn";
  static constexpr std::string_view name = "Rect";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &rn::Rect::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &rn::Rect::y,
                         offsetof( type, y ) },
      refl::StructField{ "w", &rn::Rect::w,
                         offsetof( type, w ) },
      refl::StructField{ "h", &rn::Rect::h,
                         offsetof( type, h ) },
  };
};

} // namespace refl
