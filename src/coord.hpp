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

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"
#include "errors.hpp" // TODO: remove eventually
#include "fmt-helper.hpp"
#include "typed-int.hpp"

// c++ standard library
#include <cmath>
#include <iterator>
#include <type_traits>

namespace rn {

struct Coord;
struct Delta;
struct Rect;

using ScaleX = SX;
using ScaleY = SY;

struct ND Scale {
  SX sx = 1_sx;
  SY sy = 1_sy;

  constexpr Scale( SX sx_, SY sy_ ) : sx( sx_ ), sy( sy_ ) {}

  explicit constexpr Scale( int scale ) {
    sx._ = scale;
    sy._ = scale;
  }

  constexpr Scale() : Scale( 1 ) {}

  constexpr bool operator==( Scale const& rhs ) const {
    return ( sx == rhs.sx ) && ( sy == rhs.sy );
  }
  constexpr bool operator!=( Scale const& rhs ) const {
    return ( sx != rhs.sx ) || ( sy != rhs.sy );
  }
};

// clang-format off
enum class ND e_(direction,
  nw, n, ne,
   w, c,  e,
  sw, s, se
);
// clang-format on

struct ND Delta {
  W w = 0_w;
  H h = 0_h;

  constexpr Delta() = default;
  constexpr Delta( W w_, H h_ ) : w( w_ ), h( h_ ) {}
  constexpr Delta( H h_, W w_ ) : w( w_ ), h( h_ ) {}

  constexpr bool operator==( Delta const& other ) const {
    return ( h == other.h ) && ( w == other.w );
  }

  constexpr bool operator!=( Delta const& other ) const {
    return ( h != other.h ) || ( w != other.w );
  }

  template<typename Dimension>
  auto get() const {
    if constexpr( std::is_same_v<Dimension, X> ) return w;
    if constexpr( std::is_same_v<Dimension, Y> ) return h;
    if constexpr( std::is_same_v<Dimension, W> ) return w;
    if constexpr( std::is_same_v<Dimension, H> ) return h;
  }

  Delta operator-() { return {-w, -h}; }

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

  Delta with_height( H h_ ) const { return Delta{w, h_}; }
  Delta with_width( W w_ ) const { return Delta{w_, h}; }

  // Given a grid size this will round each dimension up to the
  // nearest multiple of that size.
  Delta round_up( Scale grid_size ) const;

  // This will reduce the magnitude of each component by one.
  // E.g., if the h component is -5, it will become -4, if the w
  // component is 10, it will become 9. If a component is zero it
  // will remain zero.
  Delta trimmed_by_one() const;

  constexpr void operator*=( Scale const& scale ) {
    w *= scale.sx;
    h *= scale.sy;
  }

  void operator/=( Scale const& scale ) {
    w /= scale.sx;
    h /= scale.sy;
  }

  // Returns the length of the diagonal according to Pythagoras.
  double diagonal() const;

  Delta mirrored_vertically() const { return {w, -h}; }
  Delta mirrored_horizontally() const { return {-w, h}; }

  // Will project this delta along the given one; the length of
  // the returned delta will generally be different than this
  // one.
  Delta projected_along( Delta const& along ) const;

  // Multiply both components by the scale and round.
  Delta multiply_and_round( double scale ) const;

  // Result will be the smallest delta that encompasses both
  // this one and the parameter.
  Delta uni0n( Delta const& rhs ) const;

  // Will clamp each dimension individually to be within the
  // bounds of the given delta.
  Delta clamp( Delta const& delta ) const;

  int area() const { return w._ * h._; }
};

struct ND Coord {
  Y y = 0_y;
  X x = 0_x;

  Coord() = default;
  Coord( X x_, Y y_ ) : y( y_ ), x( x_ ) {}
  Coord( Y y_, X x_ ) : y( y_ ), x( x_ ) {}

  // Useful for generic code; allows referencing a coordinate
  // from the type.
  template<typename Dimension>
  Dimension const& coordinate() const;

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

  void operator*=( Scale const& scale ) {
    x *= scale.sx;
    y *= scale.sy;
  }

  void operator/=( Scale const& scale ) {
    x /= scale.sx;
    y /= scale.sy;
  }

  Coord operator-() const { return {-x, -y}; }

  // If this coord is outside the rect then it will be brought
  // into the rect by traveling in precisely one straight line
  // in each direction (or possibly only one direction).
  // TODO: change to clamp
  void clip( Rect const& rect );

  Coord rounded_up_to_multiple( Scale multiple ) const;
  Coord rounded_up_to_multiple( Delta multiple ) const;

  Delta distance_from_origin() const {
    return {y - 0_y, x - 0_x};
  }

  Coord moved( e_direction d ) const;
  // Find the direction from this coord to `dest`. If dest is not
  // equal or adjacent to this coord then nullopt will be re-
  // turned.
  Opt<e_direction> direction_to( Coord dest ) const;

  // Returns this coordinate with respect to a new origin.
  Coord with_new_origin( Coord new_origin ) const;

  bool is_adjacent_to( Coord other ) const;

  bool is_inside( Rect const& rect ) const;

  // True if e.g. x is in [x,x+w).
  bool is_on_border_of( Rect const& rect ) const;

  auto to_tuple() const { return std::tuple( y, x ); }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, Coord const& c ) {
    return H::combine( std::move( h ), c.to_tuple() );
  }
};

struct ND Rect {
  X x = 0_x;
  Y y = 0_y;
  W w = 0_w;
  H h = 0_h;

  static Rect from( Coord const& _1, Coord const& _2 );
  static Rect from( Coord const& coord, Delta const& delta );

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
  Dimension const& coordinate() const;

  // Useful for generic code; allows referencing a width/height
  // from the of the associated dimension, i.e., with Dimension=X
  // it will return the width of type (W).
  template<typename Dimension>
  LengthType<Dimension> const& length() const;

  // Upper left corner as a coordinate.
  Coord upper_left() const { return Coord{y, x}; }
  // Lower right corner; NOTE, this is one-past-the-end.
  Coord lower_right() const { return Coord{y + h, x + w}; }
  // Lower left corner; NOTE, this is one-past-the-end.
  Coord lower_left() const { return Coord{y + h, x}; }
  // Upper right corner; NOTE, this is one-past-the-end.
  Coord upper_right() const { return Coord{y, x + w}; }
  // Center
  Coord center() const { return Coord{y + h / 2, x + w / 2}; }

  Delta delta() const { return {w, h}; }

  // Right edge; NOTE: this is one-past-the-end.
  X right_edge() const { return {x + w}; }
  // Left edge
  X left_edge() const { return x; }
  // Right edge; NOTE: this is one-past-the-end.
  Y bottom_edge() const { return {y + h}; }
  // Left edge
  Y top_edge() const { return y; }

  // Is the rect inside another rect. "inside" means that it can
  // be fully inside, or its borders may be overlapping.
  bool is_inside( Rect const& rect ) const;

  // Returns a rect that is adjusted (without respecting
  // proportions) so that it fits inside the given rect.
  Rect clamp( Rect const& rect ) const;

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
  Rect edges_removed() const;

  Rect shifted_by( Delta const& delta ) const {
    return Rect{x + delta.w, y + delta.h, w, h};
  }
  Rect with_new_upper_left( Coord const& coord ) const {
    return Rect{coord.x, coord.y, w, h};
  }

  // Result will be the smallest rect that encompasses both
  // this one and the parameter.
  Rect uni0n( Rect const& rhs ) const;

  // Will return y*w + x if the coord is in the rect.
  Opt<int> rasterize( Coord coord );

  int area() const { return delta().area(); }

  // This iterator will iterate over all of the points in the
  // rect in a well-defined order: top to bottom, left to right.
  struct const_iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = Coord;
    using pointer           = Coord const*;
    using reference         = Coord const&;

    Coord       it;
    Rect const& rect;
    auto        operator*() {
      // TODO: can remove this check eventually.
      CHECK( it.is_inside( rect ) );
      return it;
    }
    void operator++() {
      ++it.x;
      if( it.x == rect.right_edge() ) {
        it.x = rect.left_edge();
        ++it.y;
      }
      // TODO: can remove this check eventually.
      CHECK( it == rect.lower_left() || it.is_inside( rect ) );
    }
    bool operator!=( const_iterator const& rhs ) {
      return it != rhs.it;
    }
    bool operator==( const_iterator const& rhs ) {
      return it == rhs.it;
    }
  };

  const_iterator begin() const {
    if( w == 0_w || h == 0_h ) return end();
    return {upper_left(), *this};
  }
  const_iterator end() const {
    // The "end" is the _start_ of the row that is one passed the
    // last row. This is because this will be the position of the
    // iterator after advancing it past the lower right corner of
    // the rectangle.
    return {lower_left(), *this};
  }
};

using OptCoord = std::optional<Coord>;

// Will take the delta and center it with respect to the rect and
// return the coordinate of the upper-left corner of the centered
// rect.  Note that the coord returned may be negative.
Coord centered( Delta const& delta, Rect const& rect );

// Takes the "dot product".
int inner_product( Delta const& fst, Delta const& snd );

// Same as Delta::uni0n
ND Delta max( Delta const& lhs, Delta const& rhs );

ND Delta operator-( Delta const& lhs, Delta const& rhs );
ND inline constexpr Delta operator+( Delta const& lhs,
                                     Delta const& rhs ) {
  return {lhs.w + rhs.w, lhs.h + rhs.h};
}

ND Coord operator+( Coord const& coord, Delta const& delta );
ND Coord operator+( Delta const& delta, Coord const& coord );
ND Coord operator-( Coord const& coord, Delta const& delta );
ND Delta operator-( Coord const& lhs, Coord const& rhs );

// Adding a Delta to a Rect will shift the position of the Rect.
ND Rect operator+( Rect const& rect, Delta const& delta );
ND Rect operator+( Delta const& delta, Rect const& rect );
ND Rect operator-( Rect const& rect, Delta const& delta );

ND Coord operator+( Coord const& coord, W w );
ND Coord operator+( Coord const& coord, H h );
ND Coord operator-( Coord const& coord, W w );
ND Coord operator-( Coord const& coord, H h );
void     operator+=( Coord& coord, W w );
void     operator+=( Coord& coord, H h );
void     operator-=( Coord& coord, W w );
void     operator-=( Coord& coord, H h );
void     operator-=( Coord& coord, Delta delta );
ND Delta operator+( Delta const& delta, W w );
ND Delta operator+( Delta const& delta, H h );
ND Delta operator-( Delta const& delta, W w );
ND Delta operator-( Delta const& delta, H h );
void     operator+=( Delta& delta, W w );
void     operator+=( Delta& delta, H h );
void     operator-=( Delta& delta, W w );
void     operator-=( Delta& delta, H h );

ND Coord operator*( Coord const& coord, Scale const& scale );
ND inline constexpr Delta operator*( Delta const& delta,
                                     Scale const& scale ) {
  Delta res = delta;
  res *= scale;
  return res;
}
ND Coord operator*( Scale const& scale, Coord const& coord );
ND Delta operator*( Scale const& scale, Delta const& delta );
ND Rect  operator*( Rect const& rect, Scale const& scale );
ND Rect  operator*( Scale const& scale, Rect const& rect );
ND Rect  operator/( Rect const& rect, Scale const& scale );
ND Coord operator/( Coord const& coord, Scale const& scale );
ND Delta operator/( Delta const& delta, Scale const& scale );
ND Delta operator%( Coord const& coord, Scale const& scale );
ND Delta operator%( Coord const& coord, Delta const& delta );
ND constexpr Delta operator%( Delta const& lhs,
                              Scale const& rhs ) {
  return Delta{lhs.w % rhs.sx, lhs.h % rhs.sy};
}

Scale operator*( Scale const& lhs, Scale const& rhs );
Scale operator/( Scale const& lhs, Scale const& rhs );

} // namespace rn

DEFINE_FORMAT( ::rn::Scale, "({},{})", o.sx, o.sy );
DEFINE_FORMAT( ::rn::Delta, "({},{})", o.w, o.h );
DEFINE_FORMAT( ::rn::Coord, "({},{})", o.x, o.y );
DEFINE_FORMAT( ::rn::Rect, "({},{},{},{})", o.x, o.y, o.w, o.h );

// Here  we  open up the std namespace to add a hash function
// spe- cialization for a Coord.
namespace std {
template<>
struct hash<::rn::Coord> {
  auto operator()( ::rn::Coord const& c ) const noexcept {
    // This assumes that the coordinate's components will be less
    // than 2^32. If that is violated, then this is not a good
    // hash function.
    uint64_t flat =
        ( uint64_t( c.y._ ) << 32 ) + uint64_t( c.x._ );
    return hash<uint64_t>{}( flat );
  }
};

} // namespace std
