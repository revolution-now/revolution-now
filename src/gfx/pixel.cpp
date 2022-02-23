/****************************************************************
**pixel.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Representation of an image or texture pixel color.
*
*****************************************************************/
#include "pixel.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-std.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <random>

using namespace std;

using ::base::maybe;
using ::base::nothing;

namespace gfx {

namespace {

// If highlight == true then we highlight, otherwise we shade.
pixel shift_color( pixel input, bool highlight ) {
  // These constants were chosen through experimentation so that
  // 1-10 applications of this algorithm to a color will yield a
  // nice spectrum for both dark and light colors.
  double constexpr hue_shift     = 4.0;
  double constexpr sat_shift_max = .04;
  double constexpr lit_shift_max = .04;
  double constexpr sat_mult      = .90;
  double constexpr lit_mult      = .90;
  static_assert( sat_shift_max > 0 && sat_shift_max < .3 );
  static_assert( lit_shift_max > 0 && lit_shift_max < .3 );
  static_assert( sat_mult > .7 && sat_mult < 1.0 );
  static_assert( lit_mult > .7 && lit_mult < 1.0 );
  // upper bound of 60 is arbitrary but seems reasonable.
  static_assert( hue_shift > 0 && hue_shift < 60.0 );

  auto hsl = to_HSL( input );
  // The idea here is that when we highlight we want to shift the
  // hue closer to yellow and when we shade we want to make it
  // closer to blue. So so that end we want to shift the hue
  // spectrum so that yellow is 0 and blue is +/180, that way it
  // is easier to move a hue in the right direction to get to
  // where it needs to go. Note that when we e.g. shift toward
  // yellow, we need to shift in the direction with the closest
  // distance of the color to yellow.
  auto& h = hsl.h;
  h -= 60;
  if( h > 180 ) h -= 360;
  CHECK( h >= -180 && h <= 180 );
  if( highlight ) {
    if( h > hue_shift )
      h -= hue_shift;
    else if( h < -hue_shift )
      h += hue_shift;
    else
      h = 0;
  } else {
    if( h > 0 && h < ( 180 - hue_shift ) )
      h += hue_shift;
    else if( h < 0 && h > -( 180 - hue_shift ) )
      h -= hue_shift;
    else if( h >= ( 180 - hue_shift ) ||
             h <= -( 180 - hue_shift ) )
      h = 180;
  }
  // Now shift it back
  h += 60;
  if( h < 0 ) h += 360;
  CHECK( h >= -0.01 && h <= 360.01 );
  h = std::clamp( h, 0.0, 360.0 );

  // Now for shading we want to lessen the saturation for shading
  // and do the opposite for highlighting.
  auto& s = hsl.s;
  if( highlight ) {
    auto delta = ( 1.0 - s ) * ( 1.0 - sat_mult );
    if( delta > sat_shift_max ) delta = sat_shift_max;
    s += delta;
  } else {
    auto delta = s * ( 1.0 - sat_mult );
    if( delta > sat_shift_max ) delta = sat_shift_max;
    s -= delta;
  }
  s = std::clamp( s, 0.0, 1.0 );

  // Now for lightness we want to lessen the lightness for
  // shading and do the opposite for highlighting.
  auto& l = hsl.l;
  if( highlight ) {
    auto delta = ( 1.0 - l ) * ( 1.0 - lit_mult );
    if( delta > lit_shift_max ) delta = lit_shift_max;
    l += delta;
  } else {
    auto delta = l * ( 1.0 - lit_mult );
    if( delta > lit_shift_max ) delta = lit_shift_max;
    l -= delta;
  }
  l = std::clamp( l, 0.0, 1.0 );

  return to_RGB( hsl );
}

} // namespace

/****************************************************************
** pixel
*****************************************************************/
string pixel::to_string( bool with_alpha ) const {
  if( with_alpha )
    return fmt::format( "#{:02X}{:02X}{:02X}{:02X}", r, g, b,
                        a );
  else
    return fmt::format( "#{:02X}{:02X}{:02X}", r, g, b );
}

void to_str( pixel const& o, std::string& out, base::ADL_t ) {
  out += o.to_string( /*with_alpha=*/true );
}

double pixel::luminosity() const {
  return std::sqrt( .241 * pow( r / 255.0, 2.0 ) +
                    .691 * pow( g / 255.0, 2.0 ) +
                    .068 * pow( b / 255.0, 2.0 ) );
}

// Parses a string of the form 'NNNNNN[NN]' where N is:
// [0-9a-fA-F]. The optional two digits at the end represent al-
// pha. If these are omitted then alpha will be set to 255.
maybe<pixel> pixel::parse_from_hex( string_view hex ) {
  if( hex.size() != 6 && hex.size() != 8 ) return nothing;
  auto is_valid = []( char c ) {
    return ( c >= '0' && c <= '9' ) ||
           ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' );
  };
  if( !all_of( hex.begin(), hex.end(), is_valid ) )
    return nothing;
  auto to_num = []( char c ) {
    // c must already have been validated as a hex digit!
    if( c >= '0' && c <= '9' )
      return c - '0';
    else if( c >= 'a' && c <= 'f' )
      return ( c - 'a' ) + 10;
    else
      return ( c - 'A' ) + 10;
  };
  auto vec  = vector<char>( hex.begin(), hex.end() );
  auto ns   = util::map( to_num, vec );
  auto byte = []( auto upper, auto lower ) {
    return uint8_t( upper * 16 + lower );
  };
  pixel color;
  CHECK( ns.size() == 6 || ns.size() == 8 );
  color.r = byte( ns[0], ns[1] );
  color.g = byte( ns[2], ns[3] );
  color.b = byte( ns[4], ns[5] );
  if( ns.size() == 8 )
    color.a = byte( ns[6], ns[7] );
  else
    color.a = 255;
  return color;
}

uint32_t pixel::to_uint32() const {
  uint32_t i = r;
  i <<= 8;
  i += g;
  i <<= 8;
  i += b;
  i <<= 8;
  i += a;
  return i;
}

// A random color.
pixel pixel::random() {
  MUST_IMPROVE_IMPLEMENTATION_BEFORE_USE;
  // Seed with a real random value, if available
  random_device r;

  default_random_engine e( r() );
  // Choose a random mean between 0 and 255
  uniform_int_distribution<uint8_t> uniform_dist( 0, 255 );

  return { uniform_dist( e ), uniform_dist( e ),
           uniform_dist( e ), 255 };
}

pixel pixel::with_alpha( uint8_t a_new ) const {
  auto res = *this;
  res.a    = a_new;
  return res;
}

pixel pixel::highlighted( int iterations ) const {
  if( iterations == 0 ) return *this;
  return shift_color( *this, true )
      .highlighted( iterations - 1 );
}

pixel pixel::shaded( int iterations ) const {
  if( iterations == 0 ) return *this;
  return shift_color( *this, false ).shaded( iterations - 1 );
}

pixel pixel::banana() {
  return pixel::parse_from_hex( "E4C890" ).value();
}
pixel pixel::wood() {
  return pixel::parse_from_hex( "703F24" ).value();
}

cdr::value to_canonical( cdr::converter&, pixel const& o,
                         cdr::tag_t<pixel> ) {
  return o.to_string( /*with_alpha=*/true );
}

cdr::result<pixel> from_canonical( cdr::converter&   conv,
                                   cdr::value const& v,
                                   cdr::tag_t<pixel> ) {
  UNWRAP_RETURN( hex, conv.from<string>( v ) );
  if( hex.size() != 7 && hex.size() != 9 )
    return conv.err(
        "pixel objects must be of the form `#NNNNNN[NN]` with N "
        "in 0-f." );
  if( hex[0] != '#' )
    return conv.err( "pixel objects must start with a '#'." );
  string_view digits( &hex[1], hex.length() - 1 );

  maybe<pixel> parsed = pixel::parse_from_hex( digits );
  if( !parsed.has_value() )
    return conv.err( "failed to parse color: `{}'.", digits );
  return *parsed;
}

/****************************************************************
** utils
*****************************************************************/
pixel_hsl to_HSL( pixel const& rgb ) {
  pixel_hsl hsl;
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
      hsl.h = ( gp - bp ) / delta;
    } else if( c_max == gp ) {
      hsl.h = ( ( bp - rp ) / delta + 2.0 );
    } else if( c_max == bp ) {
      hsl.h = ( ( rp - gp ) / delta + 4.0 );
    } else {
      SHOULD_NOT_BE_HERE;
    }
  }
  hsl.h *= 60;
  if( hsl.h > 360 ) hsl.h -= 360;
  if( hsl.h < 0 ) hsl.h += 360;

  // Calculate lightness.
  hsl.l = ( c_max + c_min ) / 2.0;
  // Calculate saturation.
  hsl.s = ( delta == 0.0 )
              ? 0
              : delta / ( 1.0 - fabs( 2 * hsl.l - 1 ) );
  return hsl;
}

pixel to_RGB( pixel_hsl const& hsl ) {
  // www.rapidtables.com/convert/color/hsl-to-rgb.html
  pixel rgb;
  rgb.a  = hsl.a;
  auto C = ( 1 - std::fabs( 2 * hsl.l - 1 ) ) * hsl.s;
  auto X =
      C * ( 1 - std::fabs( std::fmod( ( hsl.h / 60.0 ), 2.0 ) -
                           1 ) );
  auto   m = hsl.l - C / 2;
  double R = 0, G = 0, B = 0; // initailize to make gcc happy
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

} // namespace gfx
