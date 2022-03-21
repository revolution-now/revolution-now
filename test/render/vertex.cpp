/****************************************************************
**vertex.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-07.
*
* Description: Unit tests for the src/render/vertex.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/vertex.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::nothing;
using ::gfx::pixel;
using ::gfx::point;

TEST_CASE( "[render/vertex] SpriteVertex" ) {
  SpriteVertex         vert( point{ .x = 1, .y = 2 },
                             point{ .x = 3, .y = 4 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 0 );
  REQUIRE( gv.visible == 1 );
  REQUIRE( gv.depixelate == 0.0f );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_target_offset == gl::vec2{} );
  REQUIRE( gv.fixed_color == gl::color{} );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] SolidVertex" ) {
  SolidVertex vert(
      point{ .x = 1, .y = 2 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 1 );
  REQUIRE( gv.visible == 1 );
  REQUIRE( gv.depixelate == 0.0f );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{} );
  REQUIRE( gv.atlas_target_offset == gl::vec2{} );
  REQUIRE( gv.fixed_color == gl::color{ .r = 10.0f / 255.0f,
                                        .g = 20.0f / 255.0f,
                                        .b = 30.0f / 255.0f,
                                        .a = 40.0f / 255.0f } );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] SilhouetteVertex" ) {
  SilhouetteVertex vert(
      point{ .x = 1, .y = 2 }, point{ .x = 3, .y = 4 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 2 );
  REQUIRE( gv.visible == 1 );
  REQUIRE( gv.depixelate == 0.0f );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_target_offset == gl::vec2{} );
  REQUIRE( gv.fixed_color == gl::color{ .r = 10.0f / 255.0f,
                                        .g = 20.0f / 255.0f,
                                        .b = 30.0f / 255.0f,
                                        .a = 40.0f / 255.0f } );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] add_vertex" ) {
  SpriteVertex vert1( point{ .x = 1, .y = 2 },
                      point{ .x = 3, .y = 4 } );
  SolidVertex  vert2(
       point{ .x = 1, .y = 2 },
       pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  SilhouetteVertex vert3(
      point{ .x = 1, .y = 2 }, point{ .x = 3, .y = 4 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );

  vector<GenericVertex> vec;

  SpriteVertex& added1 = add_vertex( vec, vert1 );
  REQUIRE( added1 == vert1 );

  SolidVertex& added2 = add_vertex( vec, vert2 );
  REQUIRE( added2 == vert2 );

  SilhouetteVertex& added3 = add_vertex( vec, vert3 );
  REQUIRE( added3 == vert3 );
}

TEST_CASE( "[render/vertex] depixelation" ) {
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 } );
  REQUIRE( vert.depixelation_stage() == 0.0 );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  REQUIRE( vert.depixelation_stage() == 0.0 );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  vert.set_depixelation_stage( .5 );
  vert.set_depixelation_target( gfx::size{} );
  REQUIRE( vert.depixelation_stage() == 0.5 );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  vert.set_depixelation_stage( 1.0 );
  vert.set_depixelation_target( gfx::size{ .w = 9, .h = 10 } );
  REQUIRE( vert.depixelation_stage() == 1.0 );
  REQUIRE( vert.generic().atlas_target_offset ==
           gl::vec2{ .x = 9, .y = 10 } );
}

TEST_CASE( "[render/vertex] visibility" ) {
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 } );
  REQUIRE( vert.is_visible() );
  vert.set_visible( true );
  REQUIRE( vert.is_visible() );
  vert.set_visible( false );
  REQUIRE_FALSE( vert.is_visible() );
  vert.set_visible( true );
  REQUIRE( vert.is_visible() );
}

TEST_CASE( "[render/vertex] alpha" ) {
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 } );
  vert.reset_alpha();
  REQUIRE( vert.alpha() == 1.0 );
  vert.set_alpha( .5 );
  REQUIRE( vert.alpha() == 0.5 );
  vert.set_alpha( 0.0 );
  REQUIRE( vert.alpha() == 0.0 );
  vert.reset_alpha();
  REQUIRE( vert.alpha() == 1.0 );
}

TEST_CASE( "[render/vertex] scale_position" ) {
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 } );
  REQUIRE( vert.generic().position ==
           gl::vec2{ .x = 6, .y = 12 } );
  vert.scale_position( .5 );
  REQUIRE( vert.generic().position ==
           gl::vec2{ .x = 3, .y = 6 } );
  vert.scale_position( .55 );
  REQUIRE( vert.generic().position ==
           gl::vec2{ .x = 2, .y = 3 } );
  vert.scale_position( .5 );
  REQUIRE( vert.generic().position ==
           gl::vec2{ .x = 1, .y = 2 } );
}

TEST_CASE( "[render/vertex] translate_position" ) {
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 } );
  REQUIRE( vert.generic().position ==
           gl::vec2{ .x = 6, .y = 12 } );
  vert.translate_position( gfx::size{ .w = 2, .h = -4 } );
  REQUIRE( vert.generic().position ==
           gl::vec2{ .x = 8, .y = 8 } );
}

} // namespace
} // namespace rr
