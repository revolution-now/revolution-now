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
#include "globals.hpp"
#include "logging.hpp"
#include "sdl-util.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/io.hpp"
#include "base-util/misc.hpp"

// {fmt}
#include "fmt/format.h"

// Abseil
#include "absl/container/flat_hash_set.h"

// SDL
#include <SDL.h>

// c++ standard library
#include <algorithm>
#include <cmath>
#include <random>

using namespace std;

namespace rn {

namespace {

// Used when we sort or partition colors.  Turns out that
// colors can't be nicely sorted without partitioning, so
// the two are related.
int constexpr hue_buckets        = 8;
int constexpr saturation_buckets = 3;

// Must be a multiple of 360 which is the maximum hue value.
static_assert( 360 % hue_buckets == 0 );

// Only relevant for testing/displaying palettes.
Coord const palette_render_origin{10_y, 10_x};

// Takes a hue in [0,360) and returns a bucket index from
// [0,hue_buckets-1).  Our strategy here is that the firt
// bucket straddles the zero mark since that seems to
// produce more pleasing results..
int to_hue_bucket( double hue ) {
  // Sometimes hue can be ever-to-slightly more than max.
  CHECK( hue >= 0.0 && hue < 361.0 );
  if( hue >= 360.0 ) return hue_buckets - 1;
  // Now hue < 360.0
  double bucket = 360.0 / hue_buckets;
  // First do the shifting so that the first bucket straddles
  // zero.
  hue += bucket / 2.0;
  if( hue > 360.0 ) hue -= 360;
  // Now do the bucketing.
  hue /= 360.0;
  hue *= hue_buckets;
  // Now floor and round so that we end up with an integer
  // in the range 0, 1, 2...hue_buckets-1.
  auto floored = floor( hue );
  auto rounded = lround( floored );
  auto result  = static_cast<int>( rounded );
  CHECK( floored >= 0.0 && floored < double( hue_buckets ) );
  CHECK( rounded >= 0 && rounded < hue_buckets );
  CHECK( result >= 0 && result < hue_buckets );
  return result;
}

// Takes a value in the range [0,1] and buckets it, returning
// an integer in the range [0,buckets-1).  If 1.0 is given then
// it will be placed into the last bucket (buckets-1).
int to_bucket( double v, int buckets ) {
  // Sometimes v can be equal or ever-to-slightly more than one.
  if( v >= 1.0 ) return buckets - 1;
  // At this point v must be in [0,1).
  CHECK( v >= 0.0 && v < 1.0 );
  v *= buckets;
  // Now floor and round so that we end up with an integer
  // in the range 0, 1, 2...hue_buckets-1.
  auto floored = floor( v );
  auto rounded = lround( floored );
  auto result  = static_cast<int>( rounded );
  CHECK( floored >= 0.0 && floored < double( buckets ) );
  CHECK( rounded >= 0 && rounded < buckets );
  CHECK( result >= 0 && result < buckets );
  return result;
}

// This function is used for sorting color palettes. It will re-
// turn (l < r) but where the various components, such as hue,
// are bucketed before comparison in various ways.
bool hsl_bucketed_cmp( Color const& l, Color const& r ) {
  auto l_hsl = to_HSL( l );
  auto r_hsl = to_HSL( r );

  int l_hue_bucket = to_hue_bucket( l_hsl.h );
  int r_hue_bucket = to_hue_bucket( r_hsl.h );
  // First compare hues.
  if( l_hue_bucket != r_hue_bucket )
    return l_hue_bucket < r_hue_bucket;

  // If hues are in the same bucket then move on to saturation
  // and luminance.
  auto l_sat_bucket = to_bucket( l_hsl.s, saturation_buckets );
  auto r_sat_bucket = to_bucket( r_hsl.s, saturation_buckets );
  if( l_sat_bucket != r_sat_bucket )
    return l_sat_bucket < r_sat_bucket;

  // If saturations are not too far apart then compare luminance.
  return l.luminance() < r.luminance();
}

Color color_from( SDL_PixelFormat* fmt, Uint32 pixel ) {
  Color color{};

  /* Get Red component */
  auto temp = pixel & fmt->Rmask;  /* Isolate red component */
  temp      = temp >> fmt->Rshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Rloss; /* Expand to a full 8-bit number */
  color.r = (Uint8)temp;

  /* Get Green component */
  temp = pixel & fmt->Gmask;  /* Isolate green component */
  temp = temp >> fmt->Gshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Gloss;  /* Expand to a full 8-bit number */
  color.g = (Uint8)temp;

  /* Get Blue component */
  temp = pixel & fmt->Bmask;  /* Isolate blue component */
  temp = temp >> fmt->Bshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Bloss;  /* Expand to a full 8-bit number */
  color.b = (Uint8)temp;

  /* Get Alpha component */
  temp = pixel & fmt->Amask;  /* Isolate alpha component */
  temp = temp >> fmt->Ashift; /* Shift it down to 8-bit */
  temp = temp << fmt->Aloss;  /* Expand to a full 8-bit number */
  color.a = (Uint8)temp;

  return color;
}

void render_palette_segment( vector<Color> const& colors,
                             Coord                origin ) {
  int idx                  = 0;
  int constexpr block_size = 10;
  int constexpr row_size   = 62;
  for( auto color : colors ) {
    X x = origin.x;
    Y y = origin.y;
    x += X{( idx % row_size ) * block_size};
    y += Y{( idx / row_size ) * block_size};
    W w{block_size};
    H h{block_size};
    render_fill_rect( nullopt, color, {x, y, w, h} );
    ++idx;
  }
}

// This will reduce the number of colors by eliminitating
// approximate redundancy.  A color is considered redundant if we
// already have a color whose r/g/b components are all within the
// same chunk.
Vec<Color> coursen_impl( Vec<Color> const& colors,
                         uint8_t           chunk ) {
  // Do this in a dedicated function so that we don't have any
  // issues with the fact that a) we're using unsigned numbers
  // and b) their are only 8 bit.  This may not be necessary, but
  // just to be safe...
  auto abs_diff = []( uint8_t l, uint8_t r ) {
    if( l >= r )
      return l - r;
    else
      return r - l;
  };
  auto matches = [&abs_diff, chunk]( Color c1, Color c2 ) {
    return ( abs_diff( c1.r, c2.r ) < chunk ) &&
           ( abs_diff( c1.g, c2.g ) < chunk ) &&
           ( abs_diff( c1.b, c2.b ) < chunk );
  };
  vector<Color> result;
  for( auto c : colors ) {
    bool found_match = false;
    for( auto cp : result ) {
      if( matches( c, cp ) ) {
        found_match = true;
        break;
      }
    }
    if( found_match ) continue;
    result.push_back( c );
  }
  return result;
}

} // namespace

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

string Color::to_string( bool with_alpha ) const {
  if( with_alpha )
    return fmt::format( "#{:02X}{:02X}{:02X}{:02X}", r, g, b,
                        a );
  else
    return fmt::format( "#{:02X}{:02X}{:02X}", r, g, b );
}

double Color::luminance() const {
  return std::sqrt( .241 * pow( r / 255.0, 2.0 ) +
                    .691 * pow( g / 255.0, 2.0 ) +
                    .068 * pow( b / 255.0, 2.0 ) );
}

// Parses a string of the form 'NNNNNN[NN]' where N is:
// [0-9a-fA-F]. The optional two digits at the end represent al-
// pha. If these are omitted then alpha will be set to 255.
Opt<Color> Color::parse_from_hex( string_view hex ) {
  if( hex.size() != 6 && hex.size() != 8 ) return nullopt;
  auto is_valid = []( char c ) {
    return ( c >= '0' && c <= '9' ) ||
           ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' );
  };
  if( !all_of( hex.begin(), hex.end(), is_valid ) )
    return nullopt;
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
  Color color;
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

// A random color.
Color Color::random() {
  DIE( "this function needs to be more properly implemented "
       "before usage." );
  // Seed with a real random value, if available
  random_device r;

  default_random_engine e( r() );
  // Choose a random mean between 0 and 255
  uniform_int_distribution<uint8_t> uniform_dist( 0, 255 );

  return {uniform_dist( e ), uniform_dist( e ),
          uniform_dist( e ), 255};
}

void hsl_bucketed_sort( Vec<Color>& colors ) {
  std::sort( colors.begin(), colors.end(), hsl_bucketed_cmp );
}

vector<Color> extract_palette( string const& image ) {
  /* Extracting color components from a 32-bit color value */
  auto files = util::wildcard( image, false );

  absl::flat_hash_set<Color> colors;

  logger->info( "Loading palette from {}", image );
  SDL_Surface* surface = load_surface( image.c_str() );

  SDL_PixelFormat* fmt = surface->format;

  logger->info( "Scanning {} pixels", surface->h * surface->w );

  auto bpp = fmt->BitsPerPixel;
  logger->info( "Bits per pixel: {}", fmt->BitsPerPixel );
  CHECK( bpp == 8 || bpp == 32 || bpp == 24 );

  SDL_LockSurface( surface );

  for( int i = 0; i < surface->h; ++i ) {
    for( int j = 0; j < surface->w; ++j ) {
      Color color;
      if( bpp == 8 ) {
        Uint8 index =
            ( (Uint8*)surface->pixels )[i * surface->w + j];
        color = from_SDL( fmt->palette->colors[index] );

      } else if( bpp == 24 ) {
        Uint8* addr = ( (Uint8*)surface->pixels ) +
                      3 * ( i * surface->w + j );
        Uint32 pixel = *( (Uint32*)addr );
        color        = color_from( fmt, pixel );
      } else if( bpp == 32 ) {
        Uint32 pixel =
            ( (Uint32*)surface->pixels )[i * surface->w + j];
        color = color_from( fmt, pixel );
      }
      color.a = 255;
      colors.insert( color );
    }
  }

  logger->info( "Found {} colors", colors.size() );

  SDL_UnlockSurface( surface );
  SDL_FreeSurface( surface );

  return {colors.begin(), colors.end()};
}

Vec<Vec<Color>> partition_by_hue( Vec<Color> const& colors ) {
  // Any representation that has hue should yield the same
  // results in this function, i.e., sorting behavior with regard
  // to other attributes won't change the result of this
  // function.
  auto hsl = util::map( to_HSL, colors );
  // NOTE: we don't want the bucketed compare here.
  util::sort( hsl );

  auto segment_size = 360 / hue_buckets;

  Vec<Vec<Color>> res;
  for( int segment = 0; segment < 360;
       segment += segment_size ) {
    res.emplace_back();
    auto& back = res.back();
    // Straddle zero since empirically this seems better.
    double hue_min = segment - segment_size / 2,
           hue_max = segment + segment_size / 2;
    vector<Color> filtered;
    for( auto color : hsl ) {
      double hue = double( color.h );
      if( hue > 360.0 - segment_size / 2.0 ) hue -= 360.0;
      if( hue >= hue_min && hue < hue_max )
        back.push_back( to_RGB( color ) );
    }
  }
  return res;
}

Vec<Color> coursen( Vec<Color> const& colors, int min_count ) {
  uint8_t chunk = 64;
  while( chunk > 1 ) {
    chunk--;
    Vec<Color> res = coursen_impl( colors, chunk );
    if( int( res.size() ) >= min_count ) return res;
  }
  return colors;
}

Vec<Vec<Color>> coursen( Vec<Vec<Color>> const& partitions,
                         int                    min_count ) {
  Vec<Vec<Color>> res;
  res.reserve( partitions.size() );
  for( auto const& colors : partitions )
    res.push_back( coursen( colors, min_count ) );
  return res;
}

void show_palette( Vec<Color> const& colors ) {
  clear_texture_black( Texture() );
  render_palette_segment( colors, palette_render_origin );
  ::SDL_RenderPresent( g_renderer );
}

void show_palette( Vec<Vec<Color>> const& colors ) {
  clear_texture_black( Texture() );
  Coord origin( palette_render_origin );
  H     offset{30};
  for( auto const& partition : colors ) {
    origin.y += offset;
    render_palette_segment( partition, origin );
  }
  ::SDL_RenderPresent( g_renderer );
}

} // namespace rn
