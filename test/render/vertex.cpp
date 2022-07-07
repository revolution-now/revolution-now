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
using ::gfx::size;

TEST_CASE( "[render/vertex] SpriteVertex" ) {
  SpriteVertex         vert( point{ .x = 1, .y = 2 },
                             point{ .x = 3, .y = 4 },
                             point{ .x = 5, .y = 6 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 0 );
  REQUIRE( gv.visible == 1 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_center == gl::vec2{ .x = 5, .y = 6 } );
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
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{} );
  REQUIRE( gv.atlas_center == gl::vec2{} );
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
      point{ .x = 5, .y = 6 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 2 );
  REQUIRE( gv.visible == 1 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_center == gl::vec2{ .x = 5, .y = 6 } );
  REQUIRE( gv.atlas_target_offset == gl::vec2{} );
  REQUIRE( gv.fixed_color == gl::color{ .r = 10.0f / 255.0f,
                                        .g = 20.0f / 255.0f,
                                        .b = 30.0f / 255.0f,
                                        .a = 40.0f / 255.0f } );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] StencilVertex" ) {
  StencilVertex vert(
      point{ .x = 1, .y = 2 }, point{ .x = 3, .y = 4 },
      point{ .x = 5, .y = 6 }, size{ .w = 2, .h = 3 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 3 );
  REQUIRE( gv.visible == 1 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_center == gl::vec2{ .x = 5, .y = 6 } );
  REQUIRE( gv.atlas_target_offset ==
           gl::vec2{ .x = 2, .y = 3 } );
  REQUIRE( gv.fixed_color == gl::color{ .r = 10.0f / 255.0f,
                                        .g = 20.0f / 255.0f,
                                        .b = 30.0f / 255.0f,
                                        .a = 40.0f / 255.0f } );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] add_vertex" ) {
  SpriteVertex vert1( point{ .x = 1, .y = 2 },
                      point{ .x = 3, .y = 4 },
                      point{ .x = 5, .y = 6 } );
  SolidVertex  vert2(
       point{ .x = 1, .y = 2 },
       pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  SilhouetteVertex vert3(
      point{ .x = 1, .y = 2 }, point{ .x = 3, .y = 4 },
      point{ .x = 5, .y = 6 },
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
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
  REQUIRE( vert.depixelation_stage() == 0.0 );
  REQUIRE( vert.depixelation_anchor() == gl::vec2{} );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  REQUIRE( vert.generic().depixelate == gl::vec4{} );
  REQUIRE( vert.depixelation_stage() == 0.0 );
  REQUIRE( vert.depixelation_anchor() == gl::vec2{} );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  REQUIRE( vert.generic().depixelate == gl::vec4{} );
  vert.set_depixelation_stage( .5 );
  vert.set_depixelation_anchor( { .x = 1, .y = 2 } );
  vert.set_depixelation_inversion( false );
  REQUIRE( vert.depixelation_stage() == 0.5 );
  REQUIRE( vert.depixelation_anchor() ==
           gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  REQUIRE( vert.generic().depixelate ==
           gl::vec4{ .x = 1, .y = 2, .z = .5, .w = 0 } );
  vert.set_depixelation_stage( 1.0 );
  vert.set_depixelation_inversion( true );
  REQUIRE( vert.depixelation_stage() == 1.0 );
  REQUIRE( vert.depixelation_anchor() ==
           gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( vert.generic().atlas_target_offset == gl::vec2{} );
  REQUIRE( vert.generic().depixelate ==
           gl::vec4{ .x = 1, .y = 2, .z = 1.0, .w = 1 } );
}

TEST_CASE( "[render/vertex] visibility" ) {
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
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
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
  vert.reset_alpha();
  REQUIRE( vert.alpha() == 1.0 );
  vert.set_alpha( .5 );
  REQUIRE( vert.alpha() == 0.5 );
  vert.set_alpha( 0.0 );
  REQUIRE( vert.alpha() == 0.0 );
  vert.reset_alpha();
  REQUIRE( vert.alpha() == 1.0 );
}

TEST_CASE( "[render/vertex] scaling" ) {
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
  REQUIRE( vert.generic().scaling == 1.0 );
  vert.set_scaling( .5 );
  REQUIRE( vert.generic().scaling == .5f );
  vert.set_scaling( .55 );
  REQUIRE( vert.generic().scaling == .55f );
}

TEST_CASE( "[render/vertex] translation" ) {
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
  REQUIRE( vert.generic().translation ==
           gl::vec2{ .x = 0, .y = 0 } );
  vert.set_translation( gfx::dsize{ .w = 2, .h = -4 } );
  REQUIRE( vert.generic().translation ==
           gl::vec2{ .x = 2, .y = -4 } );
}

TEST_CASE( "[render/vertex] color_cycle" ) {
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
  REQUIRE( vert.generic().color_cycle == 0 );
  vert.set_color_cycle( true );
  REQUIRE( vert.generic().color_cycle == 1 );
  vert.set_color_cycle( false );
  REQUIRE( vert.generic().color_cycle == 0 );
}

TEST_CASE( "[render/vertex] use_camera" ) {
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     point{ .x = 5, .y = 6 } );
  REQUIRE( vert.generic().use_camera == 0 );
  vert.set_use_camera( true );
  REQUIRE( vert.generic().use_camera == 1 );
  vert.set_use_camera( false );
  REQUIRE( vert.generic().use_camera == 0 );
}

} // namespace
} // namespace rr
