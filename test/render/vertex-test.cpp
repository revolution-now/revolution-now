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

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

TEST_CASE( "[render/vertex] SpriteVertex" ) {
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 0 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.depixelate_stages == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_rect ==
           gl::vec4{ .x = 5, .y = 6, .z = 1, .w = 2 } );
  REQUIRE( gv.reference_position_1 == gl::vec2{} );
  REQUIRE( gv.reference_position_2 == gl::vec2{} );
  REQUIRE( gv.stencil_key_color == gl::color{} );
  REQUIRE( gv.fixed_color == gl::color{} );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] SolidVertex" ) {
  SolidVertex vert(
      point{ .x = 1, .y = 2 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 1 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.depixelate_stages == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{} );
  REQUIRE( gv.atlas_rect == gl::vec4{} );
  REQUIRE( gv.reference_position_1 == gl::vec2{} );
  REQUIRE( gv.reference_position_2 == gl::vec2{} );
  REQUIRE( gv.stencil_key_color == gl::color{} );
  REQUIRE( gv.fixed_color == gl::color{ .r = 10.0f / 255.0f,
                                        .g = 20.0f / 255.0f,
                                        .b = 30.0f / 255.0f,
                                        .a = 40.0f / 255.0f } );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] SpriteStencilVertex" ) {
  maybe<TxDpxl> txdpxl;
  SpriteStencilVertex vert(
      point{ .x = 1, .y = 2 }, point{ .x = 3, .y = 4 },
      rect{ .origin = point{ .x = 5, .y = 6 },
            .size   = { .w = 1, .h = 2 } },
      size{ .w = 2, .h = 3 },
      pixel{ .r = 10, .g = 20, .b = 30, .a = 40 }, txdpxl );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 2 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.depixelate_stages == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{ .x = 3, .y = 4 } );
  REQUIRE( gv.atlas_rect ==
           gl::vec4{ .x = 5, .y = 6, .z = 1, .w = 2 } );
  REQUIRE( gv.reference_position_1 ==
           gl::vec2{ .x = 2, .y = 3 } );
  REQUIRE( gv.reference_position_2 == gl::vec2{} );
  REQUIRE( gv.stencil_key_color ==
           gl::color{ .r = 10.0f / 255.0f,
                      .g = 20.0f / 255.0f,
                      .b = 30.0f / 255.0f,
                      .a = 40.0f / 255.0f } );
  REQUIRE( gv.fixed_color == gl::color{} );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] LineVertex" ) {
  LineVertex vert( point{ .x = 1, .y = 2 },
                   point{ .x = 2, .y = 3 },
                   point{ .x = 4, .y = 5 },
                   pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  GenericVertex const& gv = vert.generic();
  REQUIRE( gv.type == 3 );
  REQUIRE( gv.depixelate == gl::vec4{} );
  REQUIRE( gv.depixelate_stages == gl::vec4{} );
  REQUIRE( gv.position == gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( gv.atlas_position == gl::vec2{} );
  REQUIRE( gv.atlas_rect == gl::vec4{} );
  REQUIRE( gv.reference_position_1 ==
           gl::vec2{ .x = 2, .y = 3 } );
  REQUIRE( gv.reference_position_2 ==
           gl::vec2{ .x = 4, .y = 5 } );
  REQUIRE( gv.stencil_key_color == gl::color{} );
  REQUIRE( gv.fixed_color == gl::color{ .r = 10.0f / 255.0f,
                                        .g = 20.0f / 255.0f,
                                        .b = 30.0f / 255.0f,
                                        .a = 40.0f / 255.0f } );
  REQUIRE( gv.alpha_multiplier == 1.0f );
}

TEST_CASE( "[render/vertex] depixelation" ) {
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( vert.depixelation_stage() == 0.0 );
  REQUIRE( vert.depixelation_hash_anchor() == gl::vec2{} );
  REQUIRE( vert.depixelation_gradient() == gl::vec2{} );
  REQUIRE( vert.depixelation_stage_anchor() == gl::vec2{} );
  REQUIRE( vert.generic().reference_position_1 == gl::vec2{} );
  REQUIRE( vert.generic().reference_position_2 == gl::vec2{} );
  REQUIRE( vert.generic().depixelate == gl::vec4{} );
  REQUIRE( vert.generic().depixelate_stages == gl::vec4{} );

  vert.set_depixelation_stage( .5 );
  vert.set_depixelation_hash_anchor( { .x = 1, .y = 2 } );
  vert.set_depixelation_gradient( { .w = 4.4, .h = 5.5 } );
  vert.set_depixelation_stage_anchor( { .x = 3.3, .y = 4 } );
  vert.set_depixelation_inversion( false );
  REQUIRE( vert.depixelation_stage() == 0.5 );
  REQUIRE( vert.depixelation_hash_anchor() ==
           gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( vert.depixelation_gradient() ==
           gl::vec2{ .x = 4.4, .y = 5.5 } );
  REQUIRE( vert.generic().reference_position_1 == gl::vec2{} );
  REQUIRE( vert.generic().reference_position_2 == gl::vec2{} );
  REQUIRE( vert.generic().depixelate ==
           gl::vec4{ .x = 1, .y = 2, .z = .5, .w = 0 } );
  REQUIRE( vert.generic().depixelate_stages ==
           gl::vec4{ .x = 3.3, .y = 4, .z = 4.4, .w = 5.5 } );

  vert.set_depixelation_stage( 1.0 );
  vert.set_depixelation_gradient( {} );
  vert.set_depixelation_stage_anchor( {} );
  vert.set_depixelation_inversion( true );
  REQUIRE( vert.depixelation_stage() == 1.0 );
  REQUIRE( vert.depixelation_hash_anchor() ==
           gl::vec2{ .x = 1, .y = 2 } );
  REQUIRE( vert.depixelation_gradient() == gl::vec2{} );
  REQUIRE( vert.depixelation_stage_anchor() == gl::vec2{} );
  REQUIRE( vert.generic().reference_position_1 == gl::vec2{} );
  REQUIRE( vert.generic().reference_position_2 == gl::vec2{} );
  REQUIRE( vert.generic().depixelate ==
           gl::vec4{ .x = 1, .y = 2, .z = 1.0, .w = 1 } );
  REQUIRE( vert.generic().depixelate_stages == gl::vec4{} );
}

TEST_CASE( "[render/vertex] alpha" ) {
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 1, .y = 2 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
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
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( vert.generic().scaling == 1.0 );
  vert.set_scaling( .5 );
  REQUIRE( vert.generic().scaling == .5f );
  vert.set_scaling( .55 );
  REQUIRE( vert.generic().scaling == .55f );
}

TEST_CASE( "[render/vertex] translation1" ) {
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( vert.generic().translation1 ==
           gl::vec2{ .x = 0, .y = 0 } );
  vert.set_translation1( gfx::dsize{ .w = 2, .h = -4 } );
  REQUIRE( vert.generic().translation1 ==
           gl::vec2{ .x = 2, .y = -4 } );
}

TEST_CASE( "[render/vertex] translation2" ) {
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( vert.generic().translation2 ==
           gl::vec2{ .x = 0, .y = 0 } );
  vert.set_translation2( gfx::dsize{ .w = 2, .h = -4 } );
  REQUIRE( vert.generic().translation2 ==
           gl::vec2{ .x = 2, .y = -4 } );
}

TEST_CASE( "[render/vertex] aux_bits_1" ) {
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{}, point{}, rect{}, txdpxl );
  REQUIRE( vert.generic().aux_bits_1 == 0 );

  vert.set_color_cycle_plan( 5 );
  REQUIRE( vert.generic().aux_bits_1 == 0x00000005 );
  REQUIRE( vert.get_color_cycle_plan() == 5 );

  vert.set_downsampling_power( 6 );
  REQUIRE( vert.generic().aux_bits_1 == 0x00000065 );
  REQUIRE( vert.get_color_cycle_plan() == 5 );
  REQUIRE( vert.get_downsampling_power() == 6 );

  vert.set_downsampling_power( 8 );
  REQUIRE( vert.generic().aux_bits_1 == 0x00000005 );
  REQUIRE( vert.get_color_cycle_plan() == 5 );
  REQUIRE( vert.get_downsampling_power() == 0 );

  vert.set_downsampling_power( 7 );
  REQUIRE( vert.generic().aux_bits_1 == 0x00000075 );
  REQUIRE( vert.get_color_cycle_plan() == 5 );
  REQUIRE( vert.get_downsampling_power() == 7 );

  // Unset.
  vert.set_color_cycle_plan( 0 );
  REQUIRE( vert.generic().aux_bits_1 == 0x00000070 );
  REQUIRE( vert.get_color_cycle_plan() == 0 );
  REQUIRE( vert.get_downsampling_power() == 7 );

  vert.set_downsampling_power( 0 );
  REQUIRE( vert.generic().aux_bits_1 == 0x00000000 );
  REQUIRE( vert.get_downsampling_power() == 0 );
  REQUIRE( vert.get_downsampling_power() == 0 );
}

TEST_CASE( "[render/vertex] color_cycle" ) {
  static_assert( VERTEX_FLAG_COLOR_CYCLE == 1 );
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_COLOR_CYCLE ) ==
           0 );
  REQUIRE_FALSE( vert.get_color_cycle() );
  vert.set_color_cycle( true );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_COLOR_CYCLE ) ==
           VERTEX_FLAG_COLOR_CYCLE );
  REQUIRE( vert.get_color_cycle() );
  vert.set_color_cycle( false );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_COLOR_CYCLE ) ==
           0 );
  REQUIRE_FALSE( vert.get_color_cycle() );
}

TEST_CASE( "[render/vertex] use_camera" ) {
  static_assert( VERTEX_FLAG_USE_CAMERA == 2 );
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_USE_CAMERA ) ==
           0 );
  REQUIRE_FALSE( vert.get_use_camera() );
  vert.set_use_camera( true );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_USE_CAMERA ) ==
           VERTEX_FLAG_USE_CAMERA );
  REQUIRE( vert.get_use_camera() );
  vert.set_use_camera( false );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_USE_CAMERA ) ==
           0 );
  REQUIRE_FALSE( vert.get_use_camera() );
}

TEST_CASE( "[render/vertex] desaturate" ) {
  static_assert( VERTEX_FLAG_DESATURATE == 4 );
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_DESATURATE ) ==
           0 );
  REQUIRE_FALSE( vert.get_desaturate() );
  vert.set_desaturate( true );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_DESATURATE ) ==
           VERTEX_FLAG_DESATURATE );
  REQUIRE( vert.get_desaturate() );
  vert.set_desaturate( false );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_DESATURATE ) ==
           0 );
  REQUIRE_FALSE( vert.get_desaturate() );
}

TEST_CASE( "[render/vertex] fixed_color" ) {
  static_assert( VERTEX_FLAG_FIXED_COLOR == 8 );
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  gfx::pixel const color{ .r = 16, .g = 32, .b = 64, .a = 128 };
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_FIXED_COLOR ) ==
           0 );
  REQUIRE( vert.get_fixed_color() == nothing );
  vert.set_fixed_color( color );
  REQUIRE( vert.get_fixed_color() == color );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_FIXED_COLOR ) ==
           VERTEX_FLAG_FIXED_COLOR );
  vert.set_fixed_color( gfx::pixel{} );
  REQUIRE( vert.get_fixed_color() == gfx::pixel{} );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_FIXED_COLOR ) ==
           VERTEX_FLAG_FIXED_COLOR );
  vert.set_fixed_color( nothing );
  REQUIRE( vert.get_fixed_color() == nothing );
  REQUIRE( ( vert.generic().flags & VERTEX_FLAG_FIXED_COLOR ) ==
           0 );
}

TEST_CASE( "[render/vertex] uniform depixelation" ) {
  static_assert( VERTEX_FLAG_UNIFORM_DEPIXELATION == 16 );
  maybe<TxDpxl> txdpxl;
  SpriteVertex vert( point{ .x = 6, .y = 12 },
                     point{ .x = 3, .y = 4 },
                     rect{ .origin = point{ .x = 5, .y = 6 },
                           .size   = { .w = 1, .h = 2 } },
                     txdpxl );
  REQUIRE( ( vert.generic().flags &
             VERTEX_FLAG_UNIFORM_DEPIXELATION ) == 0 );
  REQUIRE_FALSE( vert.get_uniform_depixelation() );
  vert.set_uniform_depixelation( true );
  REQUIRE( ( vert.generic().flags &
             VERTEX_FLAG_UNIFORM_DEPIXELATION ) ==
           VERTEX_FLAG_UNIFORM_DEPIXELATION );
  REQUIRE( vert.get_uniform_depixelation() );
  vert.set_uniform_depixelation( false );
  REQUIRE( ( vert.generic().flags &
             VERTEX_FLAG_UNIFORM_DEPIXELATION ) == 0 );
  REQUIRE_FALSE( vert.get_uniform_depixelation() );
}

TEST_CASE( "[render/vertex] textured depixelation" ) {
  static_assert( VERTEX_FLAG_TEXTURED_DEPIXELATION == 32 );

  SECTION( "SpriteVertex/constructed empty" ) {
    maybe<TxDpxl> txdpxl;
    SpriteVertex vert( point{ .x = 6, .y = 12 },
                       point{ .x = 3, .y = 4 },
                       rect{ .origin = point{ .x = 5, .y = 6 },
                             .size   = { .w = 1, .h = 2 } },
                       txdpxl );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) == 0 );
    REQUIRE( vert.get_textured_depixelation() == nothing );

    vert.set_textured_depixelation( TxDpxl{
      .reference_sprite_offset = { .w = 3, .h = 4 } } );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) ==
             VERTEX_FLAG_TEXTURED_DEPIXELATION );
    REQUIRE( vert.get_textured_depixelation() ==
             TxDpxl{
               .reference_sprite_offset = { .w = 3, .h = 4 } } );
    vert.set_textured_depixelation( nothing );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) == 0 );
    REQUIRE( vert.get_textured_depixelation() == nothing );
  }

  SECTION( "SpriteVertex/constructed with" ) {
    SpriteVertex vert( point{ .x = 6, .y = 12 },
                       point{ .x = 3, .y = 4 },
                       rect{ .origin = point{ .x = 5, .y = 6 },
                             .size   = { .w = 1, .h = 2 } },
                       TxDpxl{ .reference_sprite_offset = {
                                 .w = 3, .h = 4 } } );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) ==
             VERTEX_FLAG_TEXTURED_DEPIXELATION );
    REQUIRE( vert.get_textured_depixelation() ==
             TxDpxl{
               .reference_sprite_offset = { .w = 3, .h = 4 } } );
    vert.set_textured_depixelation( nothing );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) == 0 );
    REQUIRE( vert.get_textured_depixelation() == nothing );
  }

  SECTION( "SpriteStencilVertex/constructed empty" ) {
    maybe<TxDpxl> txdpxl;
    SpriteStencilVertex vert(
        point{ .x = 6, .y = 12 }, point{ .x = 3, .y = 4 },
        rect{ .origin = point{ .x = 5, .y = 6 },
              .size   = { .w = 1, .h = 2 } },
        size{}, pixel{}, txdpxl );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) == 0 );
    REQUIRE( vert.get_textured_depixelation() == nothing );

    vert.set_textured_depixelation( TxDpxl{
      .reference_sprite_offset = { .w = 3, .h = 4 } } );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) ==
             VERTEX_FLAG_TEXTURED_DEPIXELATION );
    REQUIRE( vert.get_textured_depixelation() ==
             TxDpxl{
               .reference_sprite_offset = { .w = 3, .h = 4 } } );
    vert.set_textured_depixelation( nothing );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) == 0 );
    REQUIRE( vert.get_textured_depixelation() == nothing );
  }

  SECTION( "SpriteStencilVertex/constructed with" ) {
    SpriteStencilVertex vert(
        point{ .x = 6, .y = 12 }, point{ .x = 3, .y = 4 },
        rect{ .origin = point{ .x = 5, .y = 6 },
              .size   = { .w = 1, .h = 2 } },
        size{}, pixel{},
        TxDpxl{
          .reference_sprite_offset = { .w = 3, .h = 4 } } );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) ==
             VERTEX_FLAG_TEXTURED_DEPIXELATION );
    REQUIRE( vert.get_textured_depixelation() ==
             TxDpxl{
               .reference_sprite_offset = { .w = 3, .h = 4 } } );
    vert.set_textured_depixelation( nothing );
    REQUIRE( ( vert.generic().flags &
               VERTEX_FLAG_TEXTURED_DEPIXELATION ) == 0 );
    REQUIRE( vert.get_textured_depixelation() == nothing );
  }
}

} // namespace
} // namespace rr
