/****************************************************************
**geo-types.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-08.
*
* Description: Holds types related to world geography.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "typed-int.hpp"

namespace rn {

struct Coord;
struct Delta;

struct ND Rect {
  X x = 0_x;
  Y y = 0_y;
  W w = 0_w;
  H h = 0_h;

  static Rect from( Coord const& _1, Coord const& _2 );
  static Rect from( Coord const& coord, Delta const& delta );

  // Useful for generic code; allows referencing a coordinate
  // from the type.
  template<typename Dimension>
  Dimension const& coordinate() const;

  // Useful for generic code; allows referencing a width/height
  // from the of the associated dimension, i.e., with Dimension=X
  // it will return the width of type (W).
  template<typename Dimension>
  LengthType<Dimension> const& length() const;

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
  Rect edges_removed();

  // Result will be the smallest rect that encompasses both
  // this one and the parameter.
  Rect uni0n( Rect const& rhs ) const;
};

enum class ND direction {
  // clang-format off
  nw, n, ne,
   w, c,  e,
  sw, s, se
  // clang-format on
};

struct ND Coord {
  Y y = 0_y;
  X x = 0_x;

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

  Coord moved( direction d );
  bool  is_inside( Rect const& rect ) const;
};

using OptCoord = std::optional<Coord>;

struct ND Delta {
  W    w = 0_w;
  H    h = 0_h;
  bool operator==( Delta const& other ) const {
    return ( h == other.h ) && ( w == other.w );
  }

  Delta operator-() { return {-w, -h}; }

  void operator+=( Delta const& other ) {
    w += other.w;
    h += other.h;
  }
  // Result will be the smallest delta that encompasses both
  // this one and the parameter.
  Delta uni0n( Delta const& rhs ) const;
};

// Same as Delta::uni0n
ND Delta max( Delta const& lhs, Delta const& rhs );

ND Delta operator*( int scale, Delta const& delta );
ND Delta operator-( Delta const& lhs, Delta const& rhs );

ND Coord operator+( Coord const& coord, Delta const& delta );
ND Coord operator+( Delta const& delta, Coord const& coord );
ND Delta operator-( Coord const& lhs, Coord const& rhs );

ND Coord operator+( Coord const& coord, W w );
ND Coord operator+( Coord const& coord, H h );

std::ostream& operator<<( std::ostream& out, rn::Rect const& r );
std::ostream& operator<<( std::ostream&    out,
                          rn::Coord const& coord );

} // namespace rn
