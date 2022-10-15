/****************************************************************
**cartesian.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-26.
*
* Description: Structs for working in cartesian space.
*
*****************************************************************/
#pragma once

// refl
#include "refl/ext.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <algorithm>

namespace gfx {

struct drect;
struct rect;

/****************************************************************
** size
*****************************************************************/
// Note: this type should be passed by value for efficiency.
struct size {
  int w = 0;
  int h = 0;

  bool negative() const { return w < 0 || h < 0; }

  int area() const { return w * h; }

  size max_with( size const rhs ) const;

  // Returns a dsize; auto is used to avoid circular dependency.
  auto to_double() const;

  size operator+( size term ) const;

  void operator+=( size term );

  size operator*( int scale ) const;

  size operator/( int scale ) const;

  bool operator==( size const& ) const = default;
};

/****************************************************************
** dsize
*****************************************************************/
// Same as above (see comments) but with doubles.
struct dsize {
  double w = 0.0;
  double h = 0.0;

  bool negative() const { return w < 0 || h < 0; }

  size truncated() const {
    return size{ .w = int( w ), .h = int( h ) };
  }

  void operator+=( dsize term );

  dsize operator*( double scale ) const;

  dsize operator/( double scale ) const;

  bool operator==( dsize const& ) const = default;
};

inline auto size::to_double() const {
  return dsize{
      .w = static_cast<double>( w ),
      .h = static_cast<double>( h ),
  };
}

/****************************************************************
** point
*****************************************************************/
// Note: this type should be passed by value for efficiency.
struct point {
  int x = 0;
  int y = 0;

  size distance_from_origin() const {
    return size{ .w = x, .h = y };
  }

  static point origin();

  point point_becomes_origin( point p ) const;
  point origin_becomes_point( point p ) const;

  // Returns a dpoint; auto is used to avoid circular dependency.
  auto to_double() const;

  // If this point is outside the rect then it will be brought
  // into the rect by traveling in precisely one straight line in
  // each direction (or possibly only one direction).
  point clamped( rect const& r ) const;

  bool is_inside( rect const& r ) const;

  bool operator==( point const& ) const = default;

  void operator+=( size const s );

  size operator-( point const rhs ) const;

  point operator*( int scale ) const;

  point operator/( int scale ) const;

  point moved_left( int by = 1 ) const;
};

/****************************************************************
** dpoint
*****************************************************************/
// Same as above (see comments) but with doubles.
struct dpoint {
  double x = 0.0;
  double y = 0.0;

  dsize distance_from_origin() const {
    return dsize{ .w = x, .h = y };
  }

  point truncated() const {
    return point{ .x = int( x ), .y = int( y ) };
  }

  dpoint clamped( drect const& r ) const;

  bool is_inside( drect const& r ) const;

  dpoint point_becomes_origin( dpoint p ) const;
  dpoint origin_becomes_point( dpoint p ) const;

  dsize fmod( double d ) const;

  void operator+=( dsize s );

  void operator-=( dsize s );

  dpoint operator-( dsize s ) const;

  dsize operator-( dpoint const rhs ) const;

  dpoint operator-() const { return dpoint{ .x = -x, .y = -y }; }

  dpoint operator*( double scale ) const;

  dpoint operator/( double scale ) const;

  bool operator==( dpoint const& ) const = default;
};

inline auto point::to_double() const {
  return dpoint{ .x = static_cast<double>( x ),
                 .y = static_cast<double>( y ) };
}

/****************************************************************
** rect
*****************************************************************/
// Be careful here since the rect is not guaranteed to be in
// "normalized" form, i.e. where the `origin` member represents
// the upper left in the case that the `size` has a negative com-
// ponent.
//
// Note: this type should be passed by value for efficiency.
struct rect {
  point     origin = {}; // upper left when normalized.
  gfx::size size   = {};

  static rect from( point first, point opposite );

  int area() const { return size.area(); }

  // Is inside or touching borders.
  bool is_inside( rect const other ) const;

  // Is inside or touching border.
  bool contains( point const p ) const;

  // Returns a new rect with the same size but with origin given
  // by `p`.
  rect with_origin( point const p ) const;

  // Returns the center rounded toward 0.
  point center() const;

  // Will clip off any parts of this rect that fall outside of
  // `other`. If the entire rect falls outside of `other` then it
  // will return nothing. If the borders are just touching then
  // it will return a rect with zero area by whose length covers
  // that portion of overlapped border (i.e., it will be a
  // "line").
  [[nodiscard]] base::maybe<rect> clipped_by(
      rect const other ) const;

  rect normalized() const;

  rect clamped( rect bounds ) const;

  // Returns a drect; auto is used to avoid circular dependency.
  auto to_double() const;

  point nw() const;
  point ne() const;
  point se() const;
  point sw() const;

  int top() const;
  int bottom() const;
  int right() const;
  int left() const;

  rect point_becomes_origin( point p ) const;
  rect origin_becomes_point( point p ) const;

  rect operator*( int scale ) const;

  rect operator/( int scale ) const;

  bool operator==( rect const& ) const = default;
};

/****************************************************************
** drect
*****************************************************************/
// Same as above (see comments) but with doubles.
struct drect {
  dpoint origin = {}; // upper left when normalized.
  dsize  size   = {};

  // Will clip off any parts of this rect that fall outside of
  // `other`. If the entire rect falls outside of `other` then it
  // will return nothing. If the borders are just touching then
  // it will return a rect with zero area by whose length covers
  // that portion of overlapped border (i.e., it will be a
  // "line").
  [[nodiscard]] base::maybe<drect> clipped_by(
      drect const other ) const;

  drect normalized() const;

  dpoint nw() const;
  dpoint ne() const;
  dpoint se() const;
  dpoint sw() const;

  double top() const;
  double bottom() const;
  double right() const;
  double left() const;

  rect truncated() const;

  drect clamped( drect bounds ) const;

  drect point_becomes_origin( dpoint p ) const;
  drect origin_becomes_point( dpoint p ) const;

  drect operator*( double scale ) const;

  drect operator/( double scale ) const;

  bool operator==( drect const& ) const = default;
};

inline auto rect::to_double() const {
  return drect{
      .origin = { .x = static_cast<double>( origin.x ),
                  .y = static_cast<double>( origin.y ) },
      .size   = { .w = static_cast<double>( size.w ),
                  .h = static_cast<double>( size.h ) },
  };
}

/****************************************************************
** Free Functions
*****************************************************************/
dpoint centered_in( dsize s, drect r );

/****************************************************************
** Combining Operators
*****************************************************************/
point operator+( point const p, size const s );
point operator+( size const s, point const p );

dpoint operator+( dpoint const p, dsize const s );
dpoint operator+( dsize const s, dpoint const p );

point operator*( point const p, size const s );

} // namespace gfx

/****************************************************************
** Reflection
*****************************************************************/
namespace refl {

// Reflection info for struct size.
template<>
struct traits<gfx::size> {
  using type = gfx::size;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name = "size";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "w", &gfx::size::w,
                         offsetof( type, w ) },
      refl::StructField{ "h", &gfx::size::h,
                         offsetof( type, h ) },
  };
};

// Reflection info for struct dsize.
template<>
struct traits<gfx::dsize> {
  using type = gfx::dsize;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name = "dsize";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "w", &gfx::dsize::w,
                         offsetof( type, w ) },
      refl::StructField{ "h", &gfx::dsize::h,
                         offsetof( type, h ) },
  };
};

// Reflection info for struct point.
template<>
struct traits<gfx::point> {
  using type = gfx::point;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name = "point";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gfx::point::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gfx::point::y,
                         offsetof( type, y ) },
  };
};

// Reflection info for struct dpoint.
template<>
struct traits<gfx::dpoint> {
  using type = gfx::dpoint;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name = "dpoint";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gfx::dpoint::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gfx::dpoint::y,
                         offsetof( type, y ) },
  };
};

// Reflection info for struct rect.
template<>
struct traits<gfx::rect> {
  using type = gfx::rect;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name = "rect";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "origin", &gfx::rect::origin,
                         offsetof( type, origin ) },
      refl::StructField{ "size", &gfx::rect::size,
                         offsetof( type, size ) } };
};

// Reflection info for struct drect.
template<>
struct traits<gfx::drect> {
  using type = gfx::drect;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name = "drect";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "origin", &gfx::drect::origin,
                         offsetof( type, origin ) },
      refl::StructField{ "size", &gfx::drect::size,
                         offsetof( type, size ) } };
};

} // namespace refl
