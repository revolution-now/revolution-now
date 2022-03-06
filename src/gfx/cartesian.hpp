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

// C++ standard library
#include <algorithm>

namespace gfx {

/****************************************************************
** size
*****************************************************************/
struct size {
  int w = 0;
  int h = 0;

  bool negative() const { return w < 0 || h < 0; };

  int area() const { return w * h; }

  size max_with( size const& rhs ) const;

  bool operator==( size const& ) const = default;
};

/****************************************************************
** point
*****************************************************************/
struct point {
  int x = 0;
  int y = 0;

  size distance_from_origin() const {
    return size{ .w = x, .h = y };
  }

  static point const& origin();

  bool operator==( point const& ) const = default;
};

/****************************************************************
** rect
*****************************************************************/
// Be careful here since the rect is not guaranteed to be in
// "normalized" form, i.e. where the `origin` member represents
// the upper left in the case that the `size` has a negative com-
// ponent.
struct rect {
  point     origin = {}; // upper left when normalized.
  gfx::size size   = {};

  int area() const { return size.area(); }

  // Is inside or touching borders.
  bool is_inside( rect const& other ) const;

  // Is inside or touching border.
  bool contains( point const& p ) const;

  rect normalized() const;

  point nw() const;
  point ne() const;
  point se() const;
  point sw() const;

  int top() const;
  int bottom() const;
  int right() const;
  int left() const;

  bool operator==( rect const& ) const = default;
};

/****************************************************************
** Combining Operators
*****************************************************************/
point operator+( point const& p, size const& s );
point operator+( size const& s, point const& p );

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

} // namespace refl
