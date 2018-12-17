/****************************************************************
**color.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-17.
*
* Description: All things color.
*
*****************************************************************/
#include "color.hpp"

// Revolution Now
#include "errors.hpp"

// c++ standard library
#include <cmath>

using namespace std;

namespace rn {

namespace {} // namespace

ColorHSL to_HSL( Color const& rgb ) {
  ColorHSL hsl;
  hsl.a = rgb.a;
  // www.rapidtables.com/convert/color/rgb-to-hsl.html
  double rp    = rgb.r / 255.0;
  double gp    = rgb.g / 255.0;
  double bp    = rgb.b / 255.0;
  auto   c_max = std::max( rp, std::max( gp, bp ) );
  auto   c_min = std::min( rp, std::min( gp, bp ) );
  auto   delta = c_max - c_min;
  // Calculate hue
  if( delta == 0.0 ) {
    hsl.h = 0;
  } else {
    if( c_max == rp ) {
      hsl.h = 60.0 * std::fmod( ( gp - bp ) / delta, 6.0 );
    } else if( c_max == gp ) {
      hsl.h = 60.0 * ( ( bp - rp ) / delta + 2.0 );
    } else if( c_max == bp ) {
      hsl.h = 60.0 * ( ( rp - gp ) / delta + 4.0 );
    } else {
      SHOULD_NOT_BE_HERE;
    }
  }
  // Calculate lightness.
  hsl.l = ( c_max + c_min ) / 2.0;
  // Calculate saturation.
  hsl.s = ( delta == 0.0 )
              ? 0
              : delta / ( 1.0 - fabs( 2 * hsl.l - 1 ) );
  return hsl;
}

ColorHSV to_HSV( Color const& rgb ) {
  ColorHSV hsv;
  hsv.a = rgb.a;
  // www.rapidtables.com/convert/color/rgb-to-hsv.html
  double rp    = rgb.r / 255.0;
  double gp    = rgb.g / 255.0;
  double bp    = rgb.b / 255.0;
  auto   c_max = std::max( rp, std::max( gp, bp ) );
  auto   c_min = std::min( rp, std::min( gp, bp ) );
  auto   delta = c_max - c_min;
  // Calculate hue
  if( delta == 0.0 ) {
    hsv.h = 0;
  } else {
    if( c_max == rp ) {
      hsv.h = 60.0 * std::fmod( ( gp - bp ) / delta, 6.0 );
    } else if( c_max == gp ) {
      hsv.h = 60.0 * ( ( bp - rp ) / delta + 2.0 );
    } else if( c_max == bp ) {
      hsv.h = 60.0 * ( ( rp - gp ) / delta + 4.0 );
    } else {
      SHOULD_NOT_BE_HERE;
    }
  }
  // Calculate Value.
  hsv.v = c_max;
  // Calculate saturation.
  hsv.s = ( delta == 0.0 ) ? 0 : delta / c_max;
  return hsv;
}

Color to_RGB( ColorHSL const& hsl ) {
  // www.rapidtables.com/convert/color/hsl-to-rgb.html
  Color rgb;
  rgb.a  = hsl.a;
  auto C = ( 1 - std::fabs( 2 * hsl.l - 1 ) ) * hsl.s;
  auto X =
      C * ( 1 - std::fabs( std::fmod( ( hsl.h / 60.0 ), 2.0 ) -
                           1 ) );
  auto   m = hsl.l - C / 2;
  double R, G, B;
  // clang-format off
  if( 0   <= hsl.h && hsl.h <  60 ) { R = C; G = X; B = 0; };
  if( 60  <= hsl.h && hsl.h < 120 ) { R = X; G = C; B = 0; };
  if( 120 <= hsl.h && hsl.h < 180 ) { R = 0; G = C; B = X; };
  if( 180 <= hsl.h && hsl.h < 240 ) { R = 0; G = X; B = C; };
  if( 240 <= hsl.h && hsl.h < 300 ) { R = X; G = 0; B = C; };
  if( 300 <= hsl.h && hsl.h < 360 ) { R = C; G = 0; B = X; };
  // clang-format on
  rgb.r = ( R + m ) * 255;
  rgb.g = ( G + m ) * 255;
  rgb.b = ( B + m ) * 255;
  return rgb;
}

Color to_RGB( ColorHSV const& hsv ) {
  // www.rapidtables.com/convert/color/hsv-to-rgb.html
  Color rgb;
  rgb.a  = hsv.a;
  auto C = hsv.v * hsv.s;
  auto X = C * ( 1 - fabs( fmod( ( hsv.h / 60.0 ), 2.0 ) - 1 ) );
  auto m = hsv.v - C;
  double R, G, B;
  // clang-format off
  if( 0   <= hsv.h && hsv.h <  60 ) { R = C; G = X; B = 0; };
  if( 60  <= hsv.h && hsv.h < 120 ) { R = X; G = C; B = 0; };
  if( 120 <= hsv.h && hsv.h < 180 ) { R = 0; G = C; B = X; };
  if( 180 <= hsv.h && hsv.h < 240 ) { R = 0; G = X; B = C; };
  if( 240 <= hsv.h && hsv.h < 300 ) { R = X; G = 0; B = C; };
  if( 300 <= hsv.h && hsv.h < 360 ) { R = C; G = 0; B = X; };
  // clang-format on
  rgb.r = ( R + m ) * 255;
  rgb.g = ( G + m ) * 255;
  rgb.b = ( B + m ) * 255;
  return rgb;
}

ColorHlVS to_HlVS( Color const& rgb ) {
  auto hsv = to_HSV( rgb );
  auto lum =
      std::sqrt( .241 * rgb.r / 255.0 + .691 * rgb.g / 255.0 +
                 .068 * rgb.b / 255.0 );
  return {hsv.h, lum, hsv.v, hsv.s, rgb.a};
}

Color to_RGB( ColorHlVS const& hlvs ) {
  ColorHSV hsv{hlvs.h, hlvs.s, hlvs.v, hlvs.a};
  return to_RGB( hsv );
}

bool hlvs_bucketed_cmp( ColorHlVS const& lhs,
                        ColorHlVS const& rhs ) {
  auto to_bucket = []( ColorHlVS const& c ) {
    int constexpr buckets = 8;
    return tuple( floor( c.h / 360.0 * buckets ),
                  floor( c.l * buckets ), c.v );
  };
  return to_bucket( lhs ) < to_bucket( rhs );
}

} // namespace rn
