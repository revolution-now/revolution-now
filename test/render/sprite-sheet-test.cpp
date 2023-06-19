/****************************************************************
**sprite-sheet.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Unit tests for the src/render/sprite-sheet.*
*module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/sprite-sheet.hpp"

// render
#include "src/render/atlas.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::expect;
using ::base::maybe;
using ::base::valid;
using ::base::valid_or;
using ::gfx::image;
using ::gfx::new_empty_image;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

TEST_CASE( "[render/sprite-sheet] load_sprite_sheet" ) {
  AtlasBuilder                       builder;
  unordered_map<string, int>         atlas_ids;
  unordered_map<string, point> const names{
      // These are arbitrary names.
      { "_1", point{ .x = 2, .y = 0 } },
      { "_0", point{ .x = 1, .y = 0 } },
      { "_2", point{ .x = 0, .y = 1 } },
      { "_3", point{ .x = 1, .y = 1 } },
      { "_4", point{ .x = 2, .y = 1 } },
      { "_5", point{ .x = 0, .y = 2 } },
  };
  REQUIRE( load_sprite_sheet(
               builder,
               new_empty_image( size{ .w = 48, .h = 64 } ),
               size{ .w = 16, .h = 16 }, names,
               atlas_ids ) == valid );

  unordered_map<string, int> expected_atlas_ids{
      { "_0", 0 }, { "_1", 1 }, { "_2", 2 },
      { "_3", 3 }, { "_4", 4 }, { "_5", 5 },
  };
  REQUIRE( atlas_ids == expected_atlas_ids );
  maybe<Atlas> atlas =
      builder.build( size{ .w = 3 * 16, .h = 3 * 16 } );
  REQUIRE( atlas.has_value() );
  REQUIRE( atlas->img.size_pixels() ==
           size{ .w = 3 * 16, .h = 2 * 16 } );
  AtlasMap& m = atlas->dict;
  rect      r{ .origin = {}, .size = { .w = 16, .h = 16 } };

  r.origin = point{ .x = 0, .y = 0 };
  REQUIRE( m.lookup( 0 ) == r );
  r.origin = point{ .x = 16, .y = 0 };
  REQUIRE( m.lookup( 1 ) == r );
  r.origin = point{ .x = 32, .y = 0 };
  REQUIRE( m.lookup( 2 ) == r );
  r.origin = point{ .x = 0, .y = 16 };
  REQUIRE( m.lookup( 3 ) == r );
  r.origin = point{ .x = 16, .y = 16 };
  REQUIRE( m.lookup( 4 ) == r );
  r.origin = point{ .x = 32, .y = 16 };
  REQUIRE( m.lookup( 5 ) == r );
}

TEST_CASE(
    "[render/sprite-sheet] load_sprite_sheet out of bounds" ) {
  AtlasBuilder                       builder;
  unordered_map<string, int>         atlas_ids;
  unordered_map<string, point> const names{
      // These are arbitrary names.
      { "_1", point{ .x = 10, .y = 0 } },
      { "_0", point{ .x = 1, .y = 0 } },
      { "_2", point{ .x = 0, .y = 1 } },
      { "_3", point{ .x = 1, .y = 1 } },
      { "_4", point{ .x = 2, .y = 1 } },
      { "_5", point{ .x = 0, .y = 2 } },
  };
  REQUIRE(
      load_sprite_sheet(
          builder, new_empty_image( size{ .w = 48, .h = 64 } ),
          size{ .w = 16, .h = 16 }, names, atlas_ids ) ==
      base::invalid<string>(
          "sprite `_1' has rect "
          "gfx::rect{origin=gfx::point{x=160,y=0},size=gfx::"
          "size{w=16,h=16}} which is outside of the img of size "
          "gfx::rect{origin=gfx::point{x=0,y=0},size=gfx::size{"
          "w=48,h=64}}." ) );
}

TEST_CASE(
    "[render/sprite-sheet] load_sprite_sheet repeat name" ) {
  AtlasBuilder               builder;
  unordered_map<string, int> atlas_ids;
  atlas_ids["_0"] = 0;
  unordered_map<string, point> const names{
      // These are arbitrary names.
      { "_1", point{ .x = 0, .y = 0 } },
      { "_0", point{ .x = 1, .y = 0 } },
      { "_2", point{ .x = 0, .y = 1 } },
      { "_3", point{ .x = 1, .y = 1 } },
      { "_4", point{ .x = 2, .y = 1 } },
      { "_5", point{ .x = 0, .y = 2 } },
  };
  REQUIRE(
      load_sprite_sheet(
          builder, new_empty_image( size{ .w = 48, .h = 64 } ),
          size{ .w = 16, .h = 16 }, names, atlas_ids ) ==
      base::invalid<string>( "atlas ID map already contains a "
                             "sprite named `_0'." ) );
}

TEST_CASE( "[render/sprite-sheet] load_font_sheet" ) {
  AtlasBuilder builder;
  image        img = new_empty_image( size{ .w = 32, .h = 48 } );
  expect<AsciiFont> res =
      load_ascii_font_sheet( builder, std::move( img ) );
  REQUIRE( builder.rects().size() == 256 );
  rect src_rect{ .origin = {}, .size = { .w = 2, .h = 3 } };
  REQUIRE( builder.rects()[0] ==
           src_rect.with_origin( { .x = 0, .y = 0 } ) );
  REQUIRE( builder.rects()[1] ==
           src_rect.with_origin( { .x = 2, .y = 0 } ) );
  REQUIRE( builder.rects()[15] ==
           src_rect.with_origin( { .x = 30, .y = 0 } ) );
  REQUIRE( builder.rects()[16] ==
           src_rect.with_origin( { .x = 0, .y = 3 } ) );
  REQUIRE( builder.rects()[255] ==
           src_rect.with_origin( { .x = 30, .y = 45 } ) );
  if( !res.has_value() ) { INFO( res.error() ); }
  REQUIRE( res.has_value() );
  AsciiFont& font = *res;
  REQUIRE( font.char_size() == size{ .w = 2, .h = 3 } );
  for( int c = 0; c < 256; ++c ) {
    REQUIRE( font.atlas_id_for_char( c ) == c );
  }
  maybe<Atlas> atlas =
      builder.build( size{ .w = 2 * 256, .h = 3 } );
  REQUIRE( atlas.has_value() );
  REQUIRE( atlas->img.size_pixels() ==
           size{ .w = 2 * 256, .h = 3 } );
  for( int c = 0; c < 256; ++c ) {
    REQUIRE( atlas->dict.lookup( font.atlas_id_for_char( c ) ) ==
             rect{ .origin = { .x = c * 2, .y = 0 },
                   .size   = { .w = 2, .h = 3 } } );
  }
}

TEST_CASE(
    "[render/sprite-sheet] load_font_sheet wrong dimensions" ) {
  AtlasBuilder builder;
  image        img = new_empty_image( size{ .w = 33, .h = 48 } );
  expect<AsciiFont> res =
      load_ascii_font_sheet( builder, std::move( img ) );
  REQUIRE( !res.has_value() );
  REQUIRE(
      res.error() ==
      "ascii font sheet must have dimensions that are a "
      "multiple of 16 (instead found gfx::size{w=33,h=48})." );
}

TEST_CASE( "[render/sprite-sheet] load_font_sheet too small" ) {
  AtlasBuilder builder;
  image        img = new_empty_image( size{ .w = 0, .h = 48 } );
  expect<AsciiFont> res =
      load_ascii_font_sheet( builder, std::move( img ) );
  REQUIRE( !res.has_value() );
  REQUIRE( res.error() ==
           "ascii font sheet must have at least one pixel per "
           "character (image size is gfx::size{w=0,h=48})." );
}

} // namespace
} // namespace rr
