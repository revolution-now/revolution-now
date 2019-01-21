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
#include "config-files.hpp"
#include "errors.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "sdl-util.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/io.hpp"
#include "base-util/misc.hpp"
#include "base-util/optional.hpp"

// {fmt}
#include "fmt/format.h"

// Abseil
#include "absl/container/flat_hash_set.h"

// SDL
#include "SDL.h"

// c++ standard library
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <random>

using namespace std;

using fmt::format;

namespace rn {

namespace {

// Used when we sort or partition colors. Turns out that colors
// can't be nicely sorted without partitioning, so the two are
// related.
int constexpr hue_buckets        = 12;
int constexpr saturation_buckets = 3;
// Note that this luminosity is distinct from the `lightness`
// component in HSL. The luminosity is what is actually used to
// sort the colors after hue and saturation.
int constexpr luminosity_buckets = 15;
// When generating grey-scale palettes this is the number of
// tones between black and white.
int constexpr grey_scale_colors = 64;

// Must be a multiple of 360 which is the maximum hue value.
static_assert( 360 % hue_buckets == 0 );
// Must be a multiple of the max values of RGB.
static_assert( 256 % grey_scale_colors == 0 );

// Colors with saturation below this value will be considered to
// be part of the grey scale.
double constexpr greyscale_max_saturation = 0.08;

// This is intended to be looked up by index, where the index is
// the "hue bucket".
array<char const*, hue_buckets> constexpr hue_names{
    "red",   "orange",       "yellow",  "chartreuse_green",
    "green", "spring_green", "cyan",    "azure",
    "blue",  "violet",       "magenta", "rose",
};
static_assert( hue_names.size() == hue_buckets );

// Only relevant for testing/displaying palettes.
Coord const palette_render_origin{10_y, 10_x};

struct ColorHSL {
  double  h = 0; // hue [0..360]
  double  s = 0; // saturation [0, 1]
  double  l = 0; // lightness [0, 1]
  uint8_t a = 0; // alpha

  auto to_tuple() const { return std::tuple( h, s, l, a ); }

  bool operator<( ColorHSL const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
};

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

// Takes a hue in [0,360) and returns a bucket index from
// [0,hue_buckets-1). Our strategy here is that the firt bucket
// straddles the zero mark since that seems to produce more
// pleasing results..
int to_hue_bucket( double hue ) {
  // Sometimes hue can be ever-to-slightly more than max.
  CHECK( hue >= 0.0 && hue < 361.0 );
  if( hue >= 360.0 ) return hue_buckets - 1;
  // Now hue < 360.0
  double bucket = 360.0 / hue_buckets;
  // First do the shifting so that the first bucket straddles
  // zero.
  hue += bucket / 2.0;
  if( hue >= 360.0 ) hue -= 360;
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

using SatBucketKey = int; // saturation bucket key
using HueBucketKey = int; // hue bucket key

// Maps a color onto an indicator (`key`) that will have a unique
// value for each possible bucketing of the color's hue.
HueBucketKey hue_bucket_key( Color c ) {
  return to_hue_bucket( to_HSL( c ).h );
}

// Maps a color onto an indicator (`key`) that will have a unique
// value for each possible bucketing of the color's saturation.
HueBucketKey sat_bucket_key( Color c ) {
  return to_bucket( to_HSL( c ).s, saturation_buckets );
}

void render_palette_segment( Texture const&       tx,
                             vector<Color> const& colors,
                             Coord origin, int row_size = 64 ) {
  int idx                  = 0;
  int constexpr block_size = 10;
  for( auto color : colors ) {
    X x = origin.x;
    Y y = origin.y;
    x += X{( idx % row_size ) * block_size};
    y += Y{( idx / row_size ) * block_size};
    W w{block_size};
    H h{block_size};
    render_fill_rect( tx, color, {x, y, w, h} );
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

// If highlight == true then we highlight, otherwise we shade.
Color shift_color( Color input, bool highlight ) {
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

string Color::to_string( bool with_alpha ) const {
  if( with_alpha )
    return format( "#{:02X}{:02X}{:02X}{:02X}", r, g, b, a );
  else
    return format( "#{:02X}{:02X}{:02X}", r, g, b );
}

double Color::luminosity() const {
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
  MUST_IMPROVE_IMPLEMENTATION_BEFORE_USE;
  // Seed with a real random value, if available
  random_device r;

  default_random_engine e( r() );
  // Choose a random mean between 0 and 255
  uniform_int_distribution<uint8_t> uniform_dist( 0, 255 );

  return {uniform_dist( e ), uniform_dist( e ),
          uniform_dist( e ), 255};
}

Color Color::highlighted( int iterations ) const {
  if( iterations == 0 ) return *this;
  return shift_color( *this, true )
      .highlighted( iterations - 1 );
}

Color Color::shaded( int iterations ) const {
  if( iterations == 0 ) return *this;
  return shift_color( *this, false ).shaded( iterations - 1 );
}

void hsl_bucketed_sort( Vec<Color>& colors ) {
  util::sort_by_key( colors, L( _.luminosity() ) );
  util::stable_sort_by_key( colors, sat_bucket_key );
  util::stable_sort_by_key( colors, hue_bucket_key );
}

vector<Color> extract_palette( fs::path const& glob,
                               Opt<int>        target ) {
  /* Extracting color components from a 32-bit color value */
  auto files = util::wildcard( glob, false );
  CHECK( !files.empty(), "need at least one file" );

  absl::flat_hash_set<Color> colors;

  for( auto const& file : files ) {
    SDL_Surface*     surface = load_surface( file.c_str() );
    SDL_PixelFormat* fmt     = surface->format;

    auto bpp = fmt->BitsPerPixel;
    CHECK( bpp == 8 || bpp == 32 || bpp == 24 );

    logger->info(
        "loading palette from {} containing {} pixels and {} "
        "bits-per-pixel.",
        file.string(), surface->h * surface->w, bpp );

    if( bpp == 24 ) {
      logger->warn(
          "loading palettes from images with 24 bits-per-pixel "
          "not supported until implementation is fixed (doesn't "
          "currently work right).  Skipping image {}.",
          file.string() );
      continue;
    }

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
                        4 * ( i * surface->w + j );
          Uint32 pixel = *( (Uint32*)addr );
          color = from_SDL( color_from_pixel( fmt, pixel ) );
        } else if( bpp == 32 ) {
          Uint32 pixel =
              ( (Uint32*)surface->pixels )[i * surface->w + j];
          color = from_SDL( color_from_pixel( fmt, pixel ) );
        }
        color.a = 255;
        colors.insert( color );
      }
    }

    SDL_UnlockSurface( surface );
    SDL_FreeSurface( surface );
  }
  CHECK( !colors.empty(), "found no colors" );

  auto res = vector<Color>( colors.begin(), colors.end() );
  // Do a default RGB sort of these colors. This is not very
  // useful, but will counter the randomness (actual
  // non-determinism) of the ordering of the elements in the
  // flat_hash_set (wherein real randomness is injected to
  // prevent implicit dependence on internal ordering which is
  // not permitted by the API).
  util::sort( res );

  logger->info( "found {} colors", res.size() );

  if( target.has_value() ) {
    res = coursen( res, target.value() );
    logger->info( "coursened to {} colors", res.size() );
  }

  CHECK( !colors.empty(), "no colors remaining" );
  return res;
}

ColorBuckets hsl_bucket( Vec<Color> const& colors ) {
  auto sorted = colors;
  hsl_bucketed_sort( sorted );
  vector<vector<vector<Opt<Color>>>> bucketed;

  // First build out the full structure with nullopt's
  // everywhere.
  for( int hue = 0; hue < hue_buckets; ++hue ) {
    bucketed.emplace_back();
    auto& sat_vec = bucketed.back();
    for( int sat = 0; sat < saturation_buckets; ++sat ) {
      sat_vec.emplace_back();
      auto& lum_vec = sat_vec.back();
      for( int lum = 0; lum < luminosity_buckets; ++lum ) {
        lum_vec.emplace_back(); // nullopt
      }
    }
  }

  // Now insert the colors.
  for( Color c : colors ) {
    auto hsl        = to_HSL( c );
    auto hue_bucket = to_hue_bucket( hsl.h );
    auto sat_bucket = to_bucket( hsl.s, saturation_buckets );
    auto lum        = c.luminosity();
    auto lum_bucket = to_bucket( lum, luminosity_buckets );
    // Overwrite whatever is there.
    bucketed[hue_bucket][sat_bucket][lum_bucket] = c;
  }
  return bucketed;
}

void dump_palette( ColorBuckets const& bucketed,
                   fs::path const&     schema,
                   fs::path const&     ucl ) {
  ofstream ucl_out( ucl.string() );
  CHECK( ucl_out.good() );
  ofstream inl_out( schema.string() );
  CHECK( inl_out.good() );
  inl_out << "// Auto-Generated: DO NOT EDIT\n\n";
  ucl_out << "# Auto-Generated: DO NOT EDIT\n\n";
  inl_out << "CFG( palette,\n";
  for( int hue = 0; hue < hue_buckets; ++hue ) {
    if( hue != 0 ) {
      ucl_out << "\n";
      inl_out << "\n";
    }
    ucl_out << hue_names[hue] << " {\n";
    inl_out << "  OBJ( " << hue_names[hue] << ",\n";
    for( int sat = 0; sat < saturation_buckets; ++sat ) {
      ucl_out << "  sat" << sat << " {\n";
      inl_out << "    OBJ( sat" << sat << ",\n";
      for( int lum = 0; lum < luminosity_buckets; ++lum ) {
        if( auto c = bucketed[hue][sat][lum]; c.has_value() ) {
          auto line = format( "    lum{}: \"{}\"\n", lum,
                              c.value().to_string( false ) );
          ucl_out << line;
          inl_out << "      FLD( Color, lum" << lum << " )\n";
        }
      }
      ucl_out << "  }\n";
      inl_out << "    )\n";
    }
    ucl_out << "}\n";
    inl_out << "  )\n";
  }
  // Now do the greys.
  ucl_out << "\n";
  ucl_out << "grey {\n";
  inl_out << "  OBJ( grey,\n";
  uint8_t jump = 256 / grey_scale_colors;
  for( uint8_t n = 0; n < grey_scale_colors; ++n ) {
    auto v = n * jump;
    auto line =
        format( "  n{:02X}: \"{}\"\n", v,
                Color( v, v, v, 255 ).to_string( false ) );
    ucl_out << line;
    auto fld = format( "    FLD( Color, n{:02X} )\n", v );
    inl_out << fld;
  }
  ucl_out << "}\n";
  inl_out << "  )\n";

  inl_out << ")\n";
}

Vec<Vec<Color>> partition_by_hue( Vec<Color> const& colors ) {
  return util::split_on_idxs(
      colors, util::group_by_key( colors, hue_bucket_key ) );
}

Vec<Vec<Color>> partition_by_sat( Vec<Color> const& colors ) {
  return util::split_on_idxs(
      colors, util::group_by_key( colors, sat_bucket_key ) );
}

void remove_greys( Vec<Color>& colors ) {
  auto is_greyscale = []( Color c ) {
    auto hsl = to_HSL( c );
    return hsl.s < greyscale_max_saturation;
  };
  auto init = colors.size();
  util::remove_if( colors, is_greyscale );
  auto final = colors.size();
  logger->info( "removed {} greys", init - final );
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

void show_palette( Texture const&    tx,
                   Vec<Color> const& colors ) {
  clear_texture_black( tx );
  render_palette_segment( tx, colors, palette_render_origin,
                          64 );
  ::SDL_RenderPresent( g_renderer );
}

void show_palette( Vec<Color> const& colors ) {
  show_palette( Texture(), colors );
}

void show_palette( Texture const&      tx,
                   ColorBuckets const& colors ) {
  clear_texture_black( tx );
  Coord origin( palette_render_origin );
  H     group_offset{10};
  H     offset{10};
  for( auto const& hue : colors ) {
    for( auto const& sat : hue ) {
      auto no_null = util::cat_opts( sat );
      render_palette_segment( tx, no_null, origin );
      origin.y += offset;
    }
    origin.y += group_offset;
  }
  ::SDL_RenderPresent( g_renderer );
}

void show_color_adjustment( Color center ) {
  Vec<Color> colors;
  for( int i = 10; i >= 0; --i )
    colors.push_back( center.shaded( i ) );
  for( int i = 0; i <= 10; ++i )
    colors.push_back( center.highlighted( i ) );
  show_palette( colors );
}

void write_palette_png( fs::path const& png_file ) {
  auto tx     = create_texture( W{500}, H{480} );
  auto colors = g_palette();
  show_palette( tx, hsl_bucket( colors ) );
  save_texture_png( tx, png_file );
}

void update_palette( fs::path const& where ) {
  // int constexpr coursen_to = 4096;
  fs::path glob{where / "*.*"};
  logger->info( "updating palettes from {}", glob.string() );

  auto colors = extract_palette( glob, nullopt );
  remove_greys( colors ); // we will add greys back in later
  auto bucketed = hsl_bucket( colors );

  size_t size = 0;
  for( auto const& hue : bucketed ) {
    for( auto const& sat : hue ) {
      auto no_null = util::cat_opts( sat );
      size += no_null.size();
    }
  }
  logger->info( "total bucketed colors: {}", size );

  fs::path const inl_file{"config/palette.inl"};
  fs::path const ucl_file{"config/palette.ucl"};
  fs::path const pal_file{"assets/art/palette.png"};
  logger->info( "writing to {} and {}", inl_file.string(),
                ucl_file.string() );
  dump_palette( bucketed, inl_file, ucl_file );
  logger->info( "writing palette png image to {}",
                pal_file.string() );
  write_palette_png( pal_file );
}

void show_config_palette() {
  auto colors = g_palette();
  show_palette( Texture(), hsl_bucket( colors ) );
}

string bucket_path( Color c ) {
  auto hsl        = to_HSL( c );
  auto hue_bucket = to_hue_bucket( hsl.h );
  auto sat_bucket = to_bucket( hsl.s, saturation_buckets );
  auto lum_bucket =
      to_bucket( c.luminosity(), luminosity_buckets );
  return format( "{}.sat{}.lum{}", hue_names[hue_bucket],
                 sat_bucket, lum_bucket );
}

} // namespace rn
