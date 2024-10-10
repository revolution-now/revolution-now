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

  [[nodiscard]] bool negative() const { return w < 0 || h < 0; }

  [[nodiscard]] int area() const { return w * h; }

  [[nodiscard]] size max_with( size const rhs ) const;

  // Returns a dsize; auto is used to avoid circular dependency.
  [[nodiscard]] auto to_double() const;

  [[nodiscard]] double pythagorean() const;

  [[nodiscard]] size operator+( size term ) const;

  void operator+=( size term );

  [[nodiscard]] size operator*( int scale ) const;

  [[nodiscard]] size operator/( int scale ) const;

  [[nodiscard]] bool operator==( size const& ) const = default;
};

/****************************************************************
** dsize
*****************************************************************/
// Same as above (see comments) but with doubles.
struct dsize {
  double w = 0.0;
  double h = 0.0;

  [[nodiscard]] bool negative() const { return w < 0 || h < 0; }

  [[nodiscard]] size truncated() const {
    return size{ .w = int( w ), .h = int( h ) };
  }

  void operator+=( dsize term );

  [[nodiscard]] dsize operator*( double scale ) const;

  [[nodiscard]] dsize operator/( double scale ) const;

  [[nodiscard]] bool operator==( dsize const& ) const = default;
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

  [[nodiscard]] size distance_from_origin() const {
    return size{ .w = x, .h = y };
  }

  static point origin();

  [[nodiscard]] point point_becomes_origin( point p ) const;
  [[nodiscard]] point origin_becomes_point( point p ) const;

  // Returns a dpoint; auto is used to avoid circular dependency.
  [[nodiscard]] auto to_double() const;

  // If this point is outside the rect then it will be brought
  // into the rect by traveling in precisely one straight line in
  // each direction (or possibly only one direction).
  [[nodiscard]] point clamped( rect const& r ) const;

  [[nodiscard]] bool is_inside( rect const& r ) const;

  [[nodiscard]] bool operator==( point const& ) const = default;

  void operator+=( size const s );

  void operator-=( size const s );

  [[nodiscard]] size operator-( point const rhs ) const;

  [[nodiscard]] point operator*( int scale ) const;

  [[nodiscard]] point operator/( int scale ) const;

  [[nodiscard]] point operator/( size scale ) const;

  [[nodiscard]] point moved_left( int by = 1 ) const;
  [[nodiscard]] point moved_right( int by = 1 ) const;
  [[nodiscard]] point moved_up( int by = 1 ) const;
  [[nodiscard]] point moved_down( int by = 1 ) const;
};

/****************************************************************
** dpoint
*****************************************************************/
// Same as above (see comments) but with doubles.
struct dpoint {
  double x = 0.0;
  double y = 0.0;

  [[nodiscard]] dsize distance_from_origin() const {
    return dsize{ .w = x, .h = y };
  }

  [[nodiscard]] point truncated() const {
    return point{ .x = int( x ), .y = int( y ) };
  }

  [[nodiscard]] dpoint clamped( drect const& r ) const;

  [[nodiscard]] bool is_inside( drect const& r ) const;

  [[nodiscard]] dpoint point_becomes_origin( dpoint p ) const;
  [[nodiscard]] dpoint origin_becomes_point( dpoint p ) const;

  [[nodiscard]] dsize fmod( double d ) const;

  void operator+=( dsize s );

  void operator-=( dsize s );

  [[nodiscard]] dpoint operator-( dsize s ) const;

  [[nodiscard]] dsize operator-( dpoint const rhs ) const;

  [[nodiscard]] dpoint operator-() const {
    return dpoint{ .x = -x, .y = -y };
  }

  [[nodiscard]] dpoint operator*( double scale ) const;

  [[nodiscard]] dpoint operator/( double scale ) const;

  [[nodiscard]] bool operator==( dpoint const& ) const = default;
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

  [[nodiscard]] int area() const { return size.area(); }

  // Is inside or touching borders.
  [[nodiscard]] bool is_inside( rect const other ) const;

  // Is inside or touching border.
  [[nodiscard]] bool contains( point const p ) const;

  // New rect with the same size but with origin given by `p`.
  [[nodiscard]] rect with_origin( point const p ) const;

  // Same origin, different size.
  [[nodiscard]] rect with_size( struct size const s ) const;

  // Returns the center rounded toward 0.
  [[nodiscard]] point center() const;

  // Will clip off any parts of this rect that fall outside of
  // `other`. If the entire rect falls outside of `other` then it
  // will return nothing. If the borders are just touching then
  // it will return a rect with zero area by whose length covers
  // that portion of overlapped border (i.e., it will be a
  // "line").
  [[nodiscard]] base::maybe<rect> clipped_by(
      rect const other ) const;

  [[nodiscard]] rect normalized() const;

  [[nodiscard]] rect clamped( rect bounds ) const;

  // Returns a drect; auto is used to avoid circular dependency.
  auto to_double() const;

  [[nodiscard]] point nw() const;
  [[nodiscard]] point ne() const;
  [[nodiscard]] point se() const;
  [[nodiscard]] point sw() const;

  [[nodiscard]] int top() const;
  [[nodiscard]] int bottom() const;
  [[nodiscard]] int right() const;
  [[nodiscard]] int left() const;

  [[nodiscard]] rect with_new_right_edge( int edge ) const;
  [[nodiscard]] rect with_new_left_edge( int edge ) const;
  [[nodiscard]] rect with_new_top_edge( int edge ) const;
  [[nodiscard]] rect with_new_bottom_edge( int edge ) const;

  [[nodiscard]] rect point_becomes_origin( point p ) const;
  [[nodiscard]] rect origin_becomes_point( point p ) const;

  [[nodiscard]] rect with_border_added( int n = 1 ) const;

  [[nodiscard]] rect with_inc_size( int n = 1 ) const;
  [[nodiscard]] rect with_dec_size( int n = 1 ) const;

  // New coord equal to this one unit of edge trimmed off
  // on all sides.  That is, we will have:
  //
  //   (width,height) ==> (width-2*n,height-2*n)
  //
  // unless one of the dimensions becomes less than zero in which
  // case that dimension will be 0 in the result.
  //
  // For the (x,y) coordinates we will always have:
  //
  //   (x,y) ==> (x+n,y+n)
  //
  // unless one of the dimensions has width 0 in which case
  // that dimension will remain as-is.
  [[nodiscard]] rect with_edges_removed( int n = 1 ) const;

  [[nodiscard]] rect operator*( int scale ) const;

  [[nodiscard]] rect operator/( int scale ) const;

  [[nodiscard]] bool operator==( rect const& ) const = default;
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

  [[nodiscard]] drect normalized() const;

  [[nodiscard]] dpoint nw() const;
  [[nodiscard]] dpoint ne() const;
  [[nodiscard]] dpoint se() const;
  [[nodiscard]] dpoint sw() const;

  [[nodiscard]] double top() const;
  [[nodiscard]] double bottom() const;
  [[nodiscard]] double right() const;
  [[nodiscard]] double left() const;

  [[nodiscard]] rect truncated() const;

  [[nodiscard]] drect clamped( drect bounds ) const;

  [[nodiscard]] drect point_becomes_origin( dpoint p ) const;
  [[nodiscard]] drect origin_becomes_point( dpoint p ) const;

  [[nodiscard]] drect operator*( double scale ) const;

  [[nodiscard]] drect operator/( double scale ) const;

  [[nodiscard]] bool operator==( drect const& ) const = default;
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
[[nodiscard]] dpoint centered_in( dsize s, drect r );

[[nodiscard]] point centered_in( size s, rect r );

// Will return the upper-left coordinate of a rect obtained by
// centering the area s along the given "wall" of the rect.
[[nodiscard]] point centered_at_bottom( size const& s,
                                        rect const& r );
[[nodiscard]] point centered_at_top( size const& s,
                                     rect const& r );
[[nodiscard]] point centered_at_left( size const& s,
                                      rect const& r );
[[nodiscard]] point centered_at_right( size const& s,
                                       rect const& r );

/****************************************************************
** Combining Operators
*****************************************************************/
[[nodiscard]] point operator+( point const p, size const s );
[[nodiscard]] point operator+( size const s, point const p );

[[nodiscard]] dpoint operator+( dpoint const p, dsize const s );
[[nodiscard]] dpoint operator+( dsize const s, dpoint const p );

[[nodiscard]] point operator*( point const p, size const s );

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
  static constexpr std::string_view name       = "size";
  static constexpr bool is_sumtype_alternative = false;

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
    refl::StructField{ "w", &gfx::size::w, offsetof( type, w ) },
    refl::StructField{ "h", &gfx::size::h, offsetof( type, h ) },
  };
};

// Reflection info for struct dsize.
template<>
struct traits<gfx::dsize> {
  using type = gfx::dsize;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gfx";
  static constexpr std::string_view name       = "dsize";
  static constexpr bool is_sumtype_alternative = false;

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
  static constexpr std::string_view name       = "point";
  static constexpr bool is_sumtype_alternative = false;

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
  static constexpr std::string_view name       = "dpoint";
  static constexpr bool is_sumtype_alternative = false;

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
  static constexpr std::string_view name       = "rect";
  static constexpr bool is_sumtype_alternative = false;

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
  static constexpr std::string_view name       = "drect";
  static constexpr bool is_sumtype_alternative = false;

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
    refl::StructField{ "origin", &gfx::drect::origin,
                       offsetof( type, origin ) },
    refl::StructField{ "size", &gfx::drect::size,
                       offsetof( type, size ) } };
};

} // namespace refl

/****************************************************************
** std::hash
*****************************************************************/
namespace std {

template<>
struct hash<::gfx::point> {
  auto operator()( ::gfx::point const& c ) const noexcept {
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
