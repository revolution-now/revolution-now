/****************************************************************
**color.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-17.
*
* Description: All things color.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// c++ standard library
#include <cmath>
#include <cstdint>
#include <tuple>

namespace rn {

constexpr uint8_t alpha_opaque{255};
constexpr uint8_t max_saturation{255};

struct ColorHSL;
struct ColorHSV;
struct ColorHlVS;
struct Color;

ColorHSL  to_HSL( Color const& rgb );
ColorHSV  to_HSV( Color const& rgb );
Color     to_RGB( ColorHSL const& hsl );
Color     to_RGB( ColorHSV const& hsv );
Color     to_RGB( ColorHlVS const& hlvs );
ColorHlVS to_HlVS( Color const& rgb );

// Standard RGB form.  Can be freely passed by value.
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;

  Color() {}
  Color( uint8_t r_, uint8_t g_, uint8_t b_ )
    : r( r_ ), g( g_ ), b( b_ ), a( 255 ) {}
  Color( uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ )
    : r( r_ ), g( g_ ), b( b_ ), a( a_ ) {}

  auto to_tuple() const { return std::tuple( r, g, b, a ); }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, Color const& c ) {
    return H::combine( std::move( h ), c.to_tuple() );
  }

  bool operator<( Color const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
  bool operator==( Color const& rhs ) const {
    return to_tuple() == rhs.to_tuple();
  }

  static Color red() {
    return {max_saturation, 0, 0, alpha_opaque};
  }
  static Color green() {
    return {0, max_saturation, 0, alpha_opaque};
  }
  static Color blue() {
    return {0, 0, max_saturation, alpha_opaque};
  }
  static Color white() {
    return {max_saturation, max_saturation, max_saturation,
            alpha_opaque};
  }
  static Color black() { return {0, 0, 0, alpha_opaque}; }
};

// So that we can pass it by value.
static_assert( sizeof( Color ) == 4 );

struct ColorHSL {
  double  h; // hue [0..360]
  double  s; // saturation [0, 1]
  double  l; // lightness [0, 1]
  uint8_t a; // alpha

  auto to_tuple() const { return std::tuple( h, s, l, a ); }

  bool operator<( ColorHSL const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
};

struct ColorHSV {
  double  h; // hue [0..360]
  double  s; // saturation [0, 1]
  double  v; // value [0, 1]
  uint8_t a; // alpha

  auto to_tuple() const { return std::tuple( h, s, v, a ); }

  bool operator<( ColorHSV const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
};

// This function is used for sorting color palettes.  It
// will return (lhs < rhs) but where the various components,
// such as hue, are bucketed before comparison.
bool hlvs_bucketed_cmp( ColorHlVS const& lhs,
                        ColorHlVS const& rhs );

struct ColorHlVS {
  double  h; // hue [0..360]
  double  l; // luminosity [0, 1]
  double  v; // value [0, 1]
  double  s; // saturation [0, 1]
  uint8_t a; // alpha

  auto to_tuple() const { return std::tuple( h, l, v, s, a ); }

  bool operator<( ColorHlVS const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
};

} // namespace rn
