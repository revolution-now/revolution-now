/****************************************************************
**palette-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-17.
*
* Description: Non-game utils for palette manipulation.
*
*****************************************************************/
#include "palette-util.hpp"

// Revolution Now
#include "config-files.hpp"
#include "error.hpp"
#include "gfx.hpp"
#include "logger.hpp"
#include "maybe.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"

// base
#include "base/lambda.hpp"
#include "base/maybe-util.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/io.hpp"

// {fmt}
#include "fmt/format.h"

// SDL
#include "SDL.h"
#include "SDL_image.h"

// c++ standard library
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <random>

using namespace std;

using ::base::maybe;
using ::fmt::format;
using ::gfx::pixel;

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
Coord const palette_render_origin{ 10_y, 10_x };

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
HueBucketKey hue_bucket_key( pixel c ) {
  return to_hue_bucket( to_HSL( c ).h );
}

// Maps a color onto an indicator (`key`) that will have a unique
// value for each possible bucketing of the color's saturation.
HueBucketKey sat_bucket_key( pixel c ) {
  return to_bucket( to_HSL( c ).s, saturation_buckets );
}

void render_palette_segment( Texture&             tx,
                             vector<pixel> const& colors,
                             Coord origin, int row_size = 64 ) {
  int idx                  = 0;
  int constexpr block_size = 10;
  for( auto color : colors ) {
    X x = origin.x;
    Y y = origin.y;
    x += X{ ( idx % row_size ) * block_size };
    y += Y{ ( idx / row_size ) * block_size };
    W w{ block_size };
    H h{ block_size };
    render_fill_rect( tx, color, { x, y, w, h } );
    ++idx;
  }
}

// This will reduce the number of colors by eliminitating
// approximate redundancy.  A color is considered redundant if we
// already have a color whose r/g/b components are all within the
// same chunk.
vector<pixel> coursen_impl( vector<pixel> const& colors,
                            uint8_t              chunk ) {
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
  auto matches = [&abs_diff, chunk]( pixel c1, pixel c2 ) {
    return ( abs_diff( c1.r, c2.r ) < chunk ) &&
           ( abs_diff( c1.g, c2.g ) < chunk ) &&
           ( abs_diff( c1.b, c2.b ) < chunk );
  };
  vector<pixel> result;
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

// Takes the average of each component.
pixel mix( pixel first, pixel second ) {
  return {
      // clang-format off
    uint8_t((uint32_t(first.r)+uint32_t(second.r))/2),
    uint8_t((uint32_t(first.g)+uint32_t(second.g))/2),
    uint8_t((uint32_t(first.b)+uint32_t(second.b))/2),
    uint8_t((uint32_t(first.a)+uint32_t(second.a))/2)
      // clang-format on
  };
}

void hsl_bucketed_sort( vector<pixel>& colors ) {
  util::sort_by_key( colors, L( _.luminosity() ) );
  util::stable_sort_by_key( colors, sat_bucket_key );
  util::stable_sort_by_key( colors, hue_bucket_key );
}

vector<pixel> extract_palette( fs::path const& glob,
                               maybe<int>      target ) {
  /* Extracting color components from a 32-bit color value */
  auto files = util::wildcard( glob, false );
  CHECK( !files.empty(), "need at least one file" );

  unordered_set<pixel> colors;

  for( auto const& file : files ) {
    ::SDL_Surface* surface = ::IMG_Load( file.c_str() );
    CHECK( surface, "failed to load image: {}", file );
    SDL_PixelFormat* fmt = surface->format;

    auto bpp = fmt->BitsPerPixel;
    CHECK( bpp == 8 || bpp == 32 || bpp == 24 );

    lg.info(
        "loading palette from {} containing {} pixels and {} "
        "bits-per-pixel.",
        file.string(), surface->h * surface->w, bpp );

    if( bpp == 24 ) {
      lg.warn(
          "loading palettes from images with 24 bits-per-pixel "
          "not supported until implementation is fixed (doesn't "
          "currently work right).  Skipping image {}.",
          file.string() );
      continue;
    }

    SDL_LockSurface( surface );

    for( int i = 0; i < surface->h; ++i ) {
      for( int j = 0; j < surface->w; ++j ) {
        pixel color;
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

  auto res = vector<pixel>( colors.begin(), colors.end() );
  util::sort( res );

  lg.info( "found {} colors", res.size() );

  if( target.has_value() ) {
    res = coursen( res, target.value() );
    lg.info( "coursened to {} colors", res.size() );
  }

  CHECK( !colors.empty(), "no colors remaining" );
  return res;
}

ColorBuckets hsl_bucket( vector<pixel> const& colors ) {
  auto sorted = colors;
  hsl_bucketed_sort( sorted );
  vector<vector<vector<maybe<pixel>>>> bucketed;

  // First build out the full structure with nothing's
  // everywhere.
  for( int hue = 0; hue < hue_buckets; ++hue ) {
    bucketed.emplace_back();
    auto& sat_vec = bucketed.back();
    for( int sat = 0; sat < saturation_buckets; ++sat ) {
      sat_vec.emplace_back();
      auto& lum_vec = sat_vec.back();
      for( int lum = 0; lum < luminosity_buckets; ++lum ) {
        lum_vec.emplace_back(); // nothing
      }
    }
  }

  // Now insert the colors.
  for( pixel c : colors ) {
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
                   fs::path const&     rcl ) {
  ofstream rcl_out( rcl.string() );
  CHECK( rcl_out.good() );
  ofstream inl_out( schema.string() );
  CHECK( inl_out.good() );
  inl_out << "// Auto-Generated: DO NOT EDIT\n\n";
  rcl_out << "# Auto-Generated: DO NOT EDIT\n\n";
  inl_out << "CFG( palette,\n";
  for( int hue = 0; hue < hue_buckets; ++hue ) {
    if( hue != 0 ) {
      rcl_out << "\n";
      inl_out << "\n";
    }
    rcl_out << hue_names[hue] << " {\n";
    inl_out << "  OBJ( " << hue_names[hue] << ",\n";
    for( int sat = 0; sat < saturation_buckets; ++sat ) {
      rcl_out << "  sat" << sat << " {\n";
      inl_out << "    OBJ( sat" << sat << ",\n";
      for( int lum = 0; lum < luminosity_buckets; ++lum ) {
        if( auto c = bucketed[hue][sat][lum]; c.has_value() ) {
          auto line = format( "    lum{}: \"{}\"\n", lum,
                              c.value().to_string( false ) );
          rcl_out << line;
          inl_out << "      FLD( pixel, lum" << lum << " )\n";
        }
      }
      rcl_out << "  }\n";
      inl_out << "    )\n";
    }
    rcl_out << "}\n";
    inl_out << "  )\n";
  }
  // Now do the greys.
  rcl_out << "\n";
  rcl_out << "grey {\n";
  inl_out << "  OBJ( grey,\n";
  uint8_t jump = 256 / grey_scale_colors;
  for( uint8_t n = 0; n < grey_scale_colors; ++n ) {
    auto v    = n * jump;
    auto line = format(
        "  n{:02X}: \"{}\"\n", v,
        pixel{ uint8_t( v ), uint8_t( v ), uint8_t( v ), 255 }
            .to_string( false ) );
    rcl_out << line;
    auto fld = format( "    FLD( pixel, n{:02X} )\n", v );
    inl_out << fld;
  }
  rcl_out << "}\n";
  inl_out << "  )\n";

  inl_out << ")\n";
}

vector<vector<pixel>> partition_by_hue(
    vector<pixel> const& colors ) {
  return util::split_on_idxs(
      colors, util::group_by_key( colors, hue_bucket_key ) );
}

vector<vector<pixel>> partition_by_sat(
    vector<pixel> const& colors ) {
  return util::split_on_idxs(
      colors, util::group_by_key( colors, sat_bucket_key ) );
}

void remove_greys( vector<pixel>& colors ) {
  auto is_greyscale = []( pixel c ) {
    auto hsl = to_HSL( c );
    return hsl.s < greyscale_max_saturation;
  };
  auto init = colors.size();
  erase_if( colors, is_greyscale );
  auto final = colors.size();
  lg.info( "removed {} greys", init - final );
}

vector<pixel> coursen( vector<pixel> const& colors,
                       int                  min_count ) {
  uint8_t chunk = 64;
  while( chunk > 1 ) {
    chunk--;
    vector<pixel> res = coursen_impl( colors, chunk );
    if( int( res.size() ) >= min_count ) return res;
  }
  return colors;
}

void show_palette( Texture& tx, vector<pixel> const& colors ) {
  clear_texture_black( tx );
  render_palette_segment( tx, colors, palette_render_origin,
                          64 );
  ::SDL_RenderPresent( g_renderer );
}

void show_palette( vector<pixel> const& colors ) {
  show_palette( Texture::screen(), colors );
}

void show_palette( Texture& tx, ColorBuckets const& colors ) {
  clear_texture_black( tx );
  Coord origin( palette_render_origin );
  H     group_offset{ 10 };
  H     offset{ 10 };
  for( auto const& hue : colors ) {
    for( auto const& sat : hue ) {
      auto no_null = base::cat_maybes( sat );
      render_palette_segment( tx, no_null, origin );
      origin.y += offset;
    }
    origin.y += group_offset;
  }
  ::SDL_RenderPresent( g_renderer );
}

void show_color_adjustment( pixel center ) {
  vector<pixel> colors;
  for( int i = 10; i >= 0; --i )
    colors.push_back( center.shaded( i ) );
  for( int i = 0; i <= 10; ++i )
    colors.push_back( center.highlighted( i ) );
  show_palette( colors );
}

void write_palette_png( fs::path const& png_file ) {
  auto        tx = create_texture( Delta{ W{ 500 }, H{ 480 } } );
  auto const& colors = g_palette();
  show_palette( tx, hsl_bucket( colors ) );
  tx.save_png( png_file );
}

void update_palette( fs::path const& where ) {
  // int constexpr coursen_to = 4096;
  fs::path glob{ where / "*.*" };
  lg.info( "updating palettes from {}", glob.string() );

  auto colors = extract_palette( glob, nothing );
  remove_greys( colors ); // we will add greys back in later
  auto bucketed = hsl_bucket( colors );

  size_t size = 0;
  for( auto const& hue : bucketed ) {
    for( auto const& sat : hue ) {
      auto no_null = base::cat_maybes( sat );
      size += no_null.size();
    }
  }
  lg.info( "total bucketed colors: {}", size );

  fs::path const inl_file{ "config/palette.inl" };
  fs::path const rcl_file{ "config/palette.rcl" };
  fs::path const pal_file{ "assets/art/palette.png" };
  lg.info( "writing to {} and {}", inl_file.string(),
           rcl_file.string() );
  dump_palette( bucketed, inl_file, rcl_file );
  lg.info( "writing palette png image to {}",
           pal_file.string() );
  write_palette_png( pal_file );
}

void show_config_palette() {
  auto const& colors = g_palette();
  show_palette( Texture::screen(), hsl_bucket( colors ) );
}

string bucket_path( pixel c ) {
  auto hsl        = to_HSL( c );
  auto hue_bucket = to_hue_bucket( hsl.h );
  auto sat_bucket = to_bucket( hsl.s, saturation_buckets );
  auto lum_bucket =
      to_bucket( c.luminosity(), luminosity_buckets );
  return format( "{}.sat{}.lum{}", hue_names[hue_bucket],
                 sat_bucket, lum_bucket );
}

} // namespace rn
