/****************************************************************
**atlas.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-04.
*
* Description: Unit tests for the src/render/atlas.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/atlas.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::pixel;
using ::gfx::rect;
using ::gfx::size;
using ::gfx::testing::image_equals;
using ::gfx::testing::new_image_from_pixels;

pixel const R = pixel{ .r = 255, .g = 0, .b = 0, .a = 255 };
pixel const G = pixel{ .r = 0, .g = 255, .b = 0, .a = 255 };
pixel const B = pixel{ .r = 0, .g = 0, .b = 255, .a = 255 };
pixel const W = pixel{ .r = 255, .g = 255, .b = 255, .a = 255 };
pixel const _ = pixel{ .r = 0, .g = 0, .b = 0, .a = 0 };

TEST_CASE( "[render/atlas] empty" ) {
  AtlasBuilder builder;
  maybe<Atlas> atlas = builder.build( size{} );
  REQUIRE( atlas.has_value() );
  REQUIRE( atlas->dict.size() == 0 );
  REQUIRE( atlas->img.size_pixels() == size{} );

  atlas = builder.build( size{ .w = 5, .h = 5 } );
  REQUIRE( atlas.has_value() );
  REQUIRE( atlas->dict.size() == 0 );
  REQUIRE( atlas->img.size_pixels() == size{} );
}

TEST_CASE( "[render/atlas] single image" ) {
  AtlasBuilder builder;

  pixel img_pixels[] = {
      R, R, G, G, B, //
      R, R, G, G, B, //
      R, R, _, _, B, //
      _, _, W, W, _, //
      _, _, W, W, _, //
  };
  gfx::image img = new_image_from_pixels( size{ .w = 5, .h = 5 },
                                          img_pixels );

  AtlasBuilder::ImageBuilder img_builder =
      builder.add_image( std::move( img ) );

  int blue_id = img_builder.add_sprite(
      rect{ .origin = { .x = 4, .y = 0 },
            .size   = { .w = 1, .h = 3 } } );
  REQUIRE( blue_id == 0 );
  int green_id = img_builder.add_sprite(
      rect{ .origin = { .x = 2, .y = 0 },
            .size   = { .w = 2, .h = 2 } } );
  REQUIRE( green_id == 1 );
  int white_id = img_builder.add_sprite(
      rect{ .origin = { .x = 2, .y = 3 },
            .size   = { .w = 2, .h = 2 } } );
  REQUIRE( white_id == 2 );
  int red_id = img_builder.add_sprite(
      rect{ .origin = { .x = 0, .y = 0 },
            .size   = { .w = 2, .h = 3 } } );
  REQUIRE( red_id == 3 );

  // Try some cases that are too small to fit.
  REQUIRE( builder.build( size{ .w = 4, .h = 4 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 4, .h = 5 } ) != nothing );
  REQUIRE( builder.build( size{ .w = 4, .h = 6 } ) != nothing );
  REQUIRE( builder.build( size{ .w = 3, .h = 6 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 3, .h = 7 } ) != nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 2 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 3 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 7 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 9 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 10 } ) != nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 11 } ) != nothing );
  REQUIRE( builder.build( size{ .w = 1, .h = 1 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 0, .h = 0 } ) == nothing );

  maybe<Atlas> atlas = builder.build( size{ .w = 5, .h = 7 } );
  REQUIRE( atlas.has_value() );

  REQUIRE( atlas->img.size_pixels() == size{ .w = 5, .h = 5 } );
  pixel expected_atlas_pixels[] = {
      R, R, B, G, G, //
      R, R, B, G, G, //
      R, R, B, _, _, //
      W, W, _, _, _, //
      W, W, _, _, _, //
  };
  REQUIRE( image_equals( atlas->img, expected_atlas_pixels ) );

  REQUIRE( atlas->dict.size() == 4 );
  REQUIRE( atlas->dict.lookup( 0 ) ==
           rect{ .origin = { .x = 2, .y = 0 },
                 .size   = { .w = 1, .h = 3 } } );
  REQUIRE( atlas->dict.lookup( 1 ) ==
           rect{ .origin = { .x = 3, .y = 0 },
                 .size   = { .w = 2, .h = 2 } } );
  REQUIRE( atlas->dict.lookup( 2 ) ==
           rect{ .origin = { .x = 0, .y = 3 },
                 .size   = { .w = 2, .h = 2 } } );
  REQUIRE( atlas->dict.lookup( 3 ) ==
           rect{ .origin = { .x = 0, .y = 0 },
                 .size   = { .w = 2, .h = 3 } } );
}

TEST_CASE( "[render/atlas] multiple images" ) {
  AtlasBuilder builder;

  pixel img_pixels1[] = {
      R, R, G, G, B, //
      R, R, G, G, B, //
      R, R, _, _, B, //
      _, _, W, W, _, //
      _, _, W, W, _, //
  };
  gfx::image img1 = new_image_from_pixels(
      size{ .w = 5, .h = 5 }, img_pixels1 );

  AtlasBuilder::ImageBuilder img_builder1 =
      builder.add_image( std::move( img1 ) );

  int blue_id1 = img_builder1.add_sprite(
      rect{ .origin = { .x = 4, .y = 0 },
            .size   = { .w = 1, .h = 3 } } );
  REQUIRE( blue_id1 == 0 );
  int green_id1 = img_builder1.add_sprite(
      rect{ .origin = { .x = 2, .y = 0 },
            .size   = { .w = 2, .h = 2 } } );
  REQUIRE( green_id1 == 1 );
  int white_id1 = img_builder1.add_sprite(
      rect{ .origin = { .x = 2, .y = 3 },
            .size   = { .w = 2, .h = 2 } } );
  REQUIRE( white_id1 == 2 );
  int red_id1 = img_builder1.add_sprite(
      rect{ .origin = { .x = 0, .y = 0 },
            .size   = { .w = 2, .h = 3 } } );
  REQUIRE( red_id1 == 3 );

  pixel img_pixels2[] = {
      R, B, B, B, _, _, _, _, //
      R, B, B, B, _, _, _, _, //
      R, _, _, _, _, G, _, _, //
      R, _, _, _, _, W, W, W, //
      R, _, _, _, _, W, W, W, //
  };
  gfx::image img2 = new_image_from_pixels(
      size{ .w = 8, .h = 5 }, img_pixels2 );

  AtlasBuilder::ImageBuilder img_builder2 =
      builder.add_image( std::move( img2 ) );

  int blue_id2 = img_builder2.add_sprite(
      rect{ .origin = { .x = 1, .y = 0 },
            .size   = { .w = 3, .h = 2 } } );
  REQUIRE( blue_id2 == 4 );
  int green_id2 = img_builder2.add_sprite(
      rect{ .origin = { .x = 5, .y = 2 },
            .size   = { .w = 1, .h = 1 } } );
  REQUIRE( green_id2 == 5 );
  int white_id2 = img_builder2.add_sprite(
      rect{ .origin = { .x = 5, .y = 3 },
            .size   = { .w = 3, .h = 2 } } );
  REQUIRE( white_id2 == 6 );
  int red_id2 = img_builder2.add_sprite(
      rect{ .origin = { .x = 0, .y = 0 },
            .size   = { .w = 1, .h = 5 } } );
  REQUIRE( red_id2 == 7 );

  // Try some cases that are too small to fit.
  REQUIRE( builder.build( size{ .w = 0, .h = 0 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 0, .h = 1 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 1, .h = 1 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 2, .h = 2 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 3, .h = 3 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 4, .h = 4 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 5, .h = 5 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 6, .h = 5 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 7, .h = 5 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 8, .h = 5 } ) == nothing );
  REQUIRE( builder.build( size{ .w = 9, .h = 5 } ) != nothing );

  maybe<Atlas> atlas = builder.build( size{ .w = 11, .h = 10 } );
  REQUIRE( atlas.has_value() );

  REQUIRE( atlas->img.size_pixels() == size{ .w = 9, .h = 5 } );
  pixel expected_atlas_pixels[] = {
      R, R, R, B, B, B, B, G, G, //
      R, R, R, B, B, B, B, G, G, //
      R, R, R, B, W, W, W, W, W, //
      R, _, _, _, W, W, W, W, W, //
      R, _, _, _, _, _, _, G, _, //
  };
  REQUIRE( image_equals( atlas->img, expected_atlas_pixels ) );

  REQUIRE( atlas->dict.size() == 8 );
  REQUIRE( atlas->dict.lookup( 0 ) ==
           rect{ .origin = { .x = 3, .y = 0 },
                 .size   = { .w = 1, .h = 3 } } );
  REQUIRE( atlas->dict.lookup( 1 ) ==
           rect{ .origin = { .x = 7, .y = 0 },
                 .size   = { .w = 2, .h = 2 } } );
  REQUIRE( atlas->dict.lookup( 2 ) ==
           rect{ .origin = { .x = 7, .y = 2 },
                 .size   = { .w = 2, .h = 2 } } );
  REQUIRE( atlas->dict.lookup( 3 ) ==
           rect{ .origin = { .x = 1, .y = 0 },
                 .size   = { .w = 2, .h = 3 } } );
  REQUIRE( atlas->dict.lookup( 4 ) ==
           rect{ .origin = { .x = 4, .y = 0 },
                 .size   = { .w = 3, .h = 2 } } );
  REQUIRE( atlas->dict.lookup( 5 ) ==
           rect{ .origin = { .x = 7, .y = 4 },
                 .size   = { .w = 1, .h = 1 } } );
  REQUIRE( atlas->dict.lookup( 6 ) ==
           rect{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = 3, .h = 2 } } );
  REQUIRE( atlas->dict.lookup( 7 ) ==
           rect{ .origin = { .x = 0, .y = 0 },
                 .size   = { .w = 1, .h = 5 } } );
}

} // namespace
} // namespace rr
