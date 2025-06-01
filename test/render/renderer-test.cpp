/****************************************************************
**renderer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-19.
*
* Description: Unit tests for the src/render/renderer.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/render/renderer.hpp"

// Testing
#include "test/mocks/gl/iface.hpp"

// render
#include "render/emitter.hpp"
#include "render/painter.hpp"

// gl
#include "src/gl/shader.hpp"

// refl
#include "refl/query-enum.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using namespace ::mock::matchers;

/****************************************************************
** Vertex Shader Description
*****************************************************************/
// If you change the GenericVertex or associated shaders then you
// may have to adjust these to make the tests pass.

// (type, name, is_integral).  The precise names don't matter.
vector<tuple<int, string, bool>> const kExpectedAttributes{
  { GL_INT, "in_type", true },                         //
  { GL_UNSIGNED_INT, "in_flags", true },               //
  { GL_UNSIGNED_INT, "in_aux_bits_1", true },          //
  { GL_FLOAT_VEC4, "in_depixelate", false },           //
  { GL_FLOAT_VEC4, "in_depixelate_stages", false },    //
  { GL_FLOAT_VEC2, "in_position", false },             //
  { GL_FLOAT_VEC2, "in_atlas_position", false },       //
  { GL_FLOAT_VEC4, "in_atlas_rect", false },           //
  { GL_FLOAT_VEC2, "in_reference_position_1", false }, //
  { GL_FLOAT_VEC2, "in_reference_position_2", false }, //
  { GL_FLOAT_VEC4, "in_stencil_key_color", false },    //
  { GL_FLOAT_VEC4, "in_fixed_color", false },          //
  { GL_FLOAT, "in_alpha_multiplier", false },          //
  { GL_FLOAT, "in_scaling", false },                   //
  { GL_FLOAT_VEC2, "in_translation1", false },         //
  { GL_FLOAT_VEC2, "in_translation2", false },         //
};

void expect_bind_vertex_array( gl::MockOpenGL& mock ) {
  // Bind.
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 20 );
  mock.EXPECT__gl_BindVertexArray( 21 );
}

void expect_unbind_vertex_array( gl::MockOpenGL& mock ) {
  // Unbind.
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 21 );
  mock.EXPECT__gl_BindVertexArray( 20 );
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 20 );
}

void expect_create_vertex_array( gl::MockOpenGL& mock ) {
  // Construct VertexArrayNonTyped.
  mock.EXPECT__gl_GenVertexArrays( 1, Not( Null() ) )
      .sets_arg<1>( 21 );

  // Construct vertex buffer.
  mock.EXPECT__gl_GenBuffers( 1, Not( Null() ) )
      .sets_arg<1>( 41 );

  expect_bind_vertex_array( mock );

  // Bind vertex buffer.
  mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 40 );
  mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 41 );

  // Call to get max allowed attributes.
  mock.EXPECT__gl_GetIntegerv( GL_MAX_VERTEX_ATTRIBS,
                               Not( Null() ) )
      .sets_arg<1>( 100 )
      .times( kExpectedAttributes.size() );

  int i = 0;
  for( auto& [type, name, is_int] : kExpectedAttributes ) {
    // Register attribute i.
    if( is_int ) {
      mock.EXPECT__gl_VertexAttribIPointer(
          /*index=*/i, /*size=*/_,
          /*type=*/_, /*stride=*/
          sizeof( GenericVertex ),
          /*pointer=*/_ );
    } else {
      // Register attribute i.
      mock.EXPECT__gl_VertexAttribPointer(
          /*index=*/i, /*size=*/_, /*type=*/_,
          /*normalized=*/false, /*stride=*/
          sizeof( GenericVertex ),
          /*pointer=*/_ );
    }
    mock.EXPECT__gl_EnableVertexAttribArray( i );
    ++i;
  }

  // Unbind vertex buffer.
  mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 41 );
  mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 40 );
  mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 40 );

  // Unbind vertex array.
  expect_unbind_vertex_array( mock );

  // Some cleanup.
  mock.EXPECT__gl_DeleteBuffers( 1, Pointee( 41 ) );
  mock.EXPECT__gl_DeleteVertexArrays( 1, Pointee( 21 ) );
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[render/renderer] workflows" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().by_default().returns( GL_NO_ERROR );

  // Create vertex shader (normal).
  mock.EXPECT__gl_CreateShader( GL_VERTEX_SHADER ).returns( 5 );
  mock.EXPECT__gl_ShaderSource(
      5, 1, Pointee( StrContains( "gl_Position" ) ), nullptr );
  mock.EXPECT__gl_CompileShader( 5 );
  mock.EXPECT__gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  // Create fragment shader (normal).
  mock.EXPECT__gl_CreateShader( GL_FRAGMENT_SHADER )
      .returns( 6 );
  mock.EXPECT__gl_ShaderSource(
      6, 1, Pointee( StrContains( "final_color" ) ), nullptr );
  mock.EXPECT__gl_CompileShader( 6 );
  mock.EXPECT__gl_GetShaderiv( 6, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  // Create vertex shader (postprocessing).
  mock.EXPECT__gl_CreateShader( GL_VERTEX_SHADER ).returns( 7 );
  mock.EXPECT__gl_ShaderSource(
      7, 1, Pointee( StrContains( "gl_Position" ) ), nullptr );
  mock.EXPECT__gl_CompileShader( 7 );
  mock.EXPECT__gl_GetShaderiv( 7, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  // Create fragment shader (postprocessing).
  mock.EXPECT__gl_CreateShader( GL_FRAGMENT_SHADER )
      .returns( 8 );
  mock.EXPECT__gl_ShaderSource(
      8, 1, Pointee( StrContains( "final_color" ) ), nullptr );
  mock.EXPECT__gl_CompileShader( 8 );
  mock.EXPECT__gl_GetShaderiv( 8, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  // Delete the shaders.
  mock.EXPECT__gl_DeleteShader( 8 );
  mock.EXPECT__gl_DeleteShader( 7 );
  mock.EXPECT__gl_DeleteShader( 6 );
  mock.EXPECT__gl_DeleteShader( 5 );

  // Create vertex arrays for each render buffer.
  for( int i = 0; i < refl::enum_count<e_render_buffer>; ++i )
    expect_create_vertex_array( mock );

  // Create shader programs.

  // Bind dummy vertex array.
  expect_bind_vertex_array( mock );

  // Create ProgramNonTyped (Normal).
  mock.EXPECT__gl_CreateProgram().returns( 9 );

  mock.EXPECT__gl_AttachShader( 9, 5 );
  mock.EXPECT__gl_AttachShader( 9, 6 );
  mock.EXPECT__gl_LinkProgram( 9 );

  mock.EXPECT__gl_GetProgramiv( 9, GL_LINK_STATUS,
                                Not( Null() ) )
      .sets_arg<2>( 1 );
  mock.EXPECT__gl_ValidateProgram( 9 );
  mock.EXPECT__gl_GetProgramiv( 9, GL_VALIDATE_STATUS,
                                Not( Null() ) )
      .sets_arg<2>( GL_TRUE );
  mock.EXPECT__gl_GetProgramInfoLog( 9, 512, Not( Null() ),
                                     Not( Null() ) )
      .sets_arg<2>( 0 );
  mock.EXPECT__gl_DetachShader( 9, 6 );
  mock.EXPECT__gl_DetachShader( 9, 5 );

  // Create uniforms (Normal).

  mock.EXPECT__gl_GetUniformLocation( 9,
                                      Eq<string>( "u_atlas" ) )
      .returns( 88 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_atlas_size" ) )
      .returns( 89 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_screen_size" ) )
      .returns( 90 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_color_cycle_stage" ) )
      .returns( 91 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_camera_translation" ) )
      .returns( 92 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_camera_zoom" ) )
      .returns( 93 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_depixelation_stage" ) )
      .returns( 94 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_color_cycle_targets" ) )
      .returns( 95 );
  mock.EXPECT__gl_GetUniformLocation(
          9, Eq<string>( "u_color_cycle_keys" ) )
      .returns( 96 );

  // Validate the program (Normal).
  mock.EXPECT__gl_GetProgramiv( 9, GL_ACTIVE_ATTRIBUTES,
                                Not( Null() ) )
      .sets_arg<2>( kExpectedAttributes.size() );
  int idx = 0;
  for( auto& [type, name, is_int] : kExpectedAttributes ) {
    string name_w_zero = name;
    name_w_zero.push_back( '\0' );

    mock.EXPECT__gl_GetActiveAttrib( 9, idx, 256, Not( Null() ),
                                     Not( Null() ),
                                     Not( Null() ),
                                     Not( Null() ) )
        .sets_arg<3>( name.size() + 1 )
        .sets_arg<4>( 0 ) // size, unused
        .sets_arg<5>( type )
        .sets_arg_array<6>( name_w_zero );
    mock.EXPECT__gl_GetAttribLocation(
            9, Eq<string>( string( name ) ) )
        .returns( idx );
    ++idx;
  }

  // Try setting the uniforms to check their type (Normal).
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform1i( 88, 0 ); // u_atlas
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 89, 0.0, 0.0 ); // u_atlas_size
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 90, 0.0, 0.0 ); // u_screen_size
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform1i( 91, 0 ); // u_color_cycle_stage
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 92, 0.0,
                             0.0 ); // u_camera_translation
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform1f( 93, 0.0 ); // u_camera_zoom
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform1f( 94, 0.0 ); // u_depixelation_stage
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform4iv(
      95, 0, /*values=*/_ ); // u_color_cycle_targets
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform3iv(
      96, 0, /*values=*/_ ); // u_color_cycle_keys

  // Unbind dummy vertex array (Normal).
  expect_unbind_vertex_array( mock );

  // Set the u_atlas texture to zero.
  // NOTE: this is omitted even though the renderer does it be-
  // cause we are setting the same value that was already set
  // above and so the cached value is used.
  //  mock.EXPECT__gl_UseProgram( 9 );
  //  mock.EXPECT__gl_Uniform1i( 88, 0 );

  // Bind dummy vertex array.
  expect_bind_vertex_array( mock );

  // Create ProgramNonTyped (Postprocessing).
  mock.EXPECT__gl_CreateProgram().returns( 10 );

  mock.EXPECT__gl_AttachShader( 10, 7 );
  mock.EXPECT__gl_AttachShader( 10, 8 );
  mock.EXPECT__gl_LinkProgram( 10 );

  mock.EXPECT__gl_GetProgramiv( 10, GL_LINK_STATUS,
                                Not( Null() ) )
      .sets_arg<2>( 1 );
  mock.EXPECT__gl_ValidateProgram( 10 );
  mock.EXPECT__gl_GetProgramiv( 10, GL_VALIDATE_STATUS,
                                Not( Null() ) )
      .sets_arg<2>( GL_TRUE );
  mock.EXPECT__gl_GetProgramInfoLog( 10, 512, Not( Null() ),
                                     Not( Null() ) )
      .sets_arg<2>( 0 );
  mock.EXPECT__gl_DetachShader( 10, 8 );
  mock.EXPECT__gl_DetachShader( 10, 7 );

  // Create uniforms (Postprocessing).

  mock.EXPECT__gl_GetUniformLocation( 10,
                                      Eq<string>( "u_source" ) )
      .returns( 88 );
  mock.EXPECT__gl_GetUniformLocation(
          10, Eq<string>( "u_screen_size" ) )
      .returns( 90 );

  // Validate the program (Postprocessing).
  mock.EXPECT__gl_GetProgramiv( 10, GL_ACTIVE_ATTRIBUTES,
                                Not( Null() ) )
      .sets_arg<2>( kExpectedAttributes.size() );
  idx = 0;
  for( auto& [type, name, is_int] : kExpectedAttributes ) {
    string name_w_zero = name;
    name_w_zero.push_back( '\0' );

    mock.EXPECT__gl_GetActiveAttrib( 10, idx, 256, Not( Null() ),
                                     Not( Null() ),
                                     Not( Null() ),
                                     Not( Null() ) )
        .sets_arg<3>( name.size() + 1 )
        .sets_arg<4>( 0 ) // size, unused
        .sets_arg<5>( type )
        .sets_arg_array<6>( name_w_zero );
    mock.EXPECT__gl_GetAttribLocation(
            10, Eq<string>( string( name ) ) )
        .returns( idx );
    ++idx;
  }

  // Try setting the uniforms to check their type
  // (Postprocessing).
  mock.EXPECT__gl_UseProgram( 10 );
  mock.EXPECT__gl_Uniform1i( 88, 0 ); // u_source
  mock.EXPECT__gl_UseProgram( 10 );
  mock.EXPECT__gl_Uniform2f( 90, 0.0, 0.0 ); // u_screen_size

  // Unbind dummy vertex array (Postprocessing).
  expect_unbind_vertex_array( mock );

  // Release the programs.
  mock.EXPECT__gl_DeleteProgram( 10 );
  mock.EXPECT__gl_DeleteProgram( 9 );

  // Set the u_color_cycle_targets uniform.
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform4iv( 95, 0, /*values=*/_ );

  // Set the u_color_cycle_keys uniform.
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform3iv( 96, 0, /*values=*/_ );

  // Set the u_screen_size texture.
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 90, 500.0, 400.0 );
  mock.EXPECT__gl_UseProgram( 10 );
  mock.EXPECT__gl_Uniform2f( 90, 500.0, 400.0 );

  // Create the atlas texture.
  auto expect_bind_tx = [&]( int const last, int const new_ ) {
    mock.EXPECT__gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                 Not( Null() ) )
        .sets_arg<1>( last );
    mock.EXPECT__gl_BindTexture( GL_TEXTURE_2D, new_ );
  };
  auto expect_unbind_tx = [&]( int const new_, int const last ) {
    mock.EXPECT__gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                 Not( Null() ) )
        .sets_arg<1>( new_ );
    mock.EXPECT__gl_BindTexture( GL_TEXTURE_2D, last );
    mock.EXPECT__gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                 Not( Null() ) )
        .sets_arg<1>( last );
  };

  auto expect_bind_fb = [&]( int const last, int const new_ ) {
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( last );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, new_ );
  };
  auto expect_unbind_fb = [&]( int const new_, int const last ) {
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( new_ );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, last );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( last );
  };

  auto const gen_tx = [&]( int const last, int const new_ ) {
    mock.EXPECT__gl_GenTextures( 1, Not( Null() ) )
        .sets_arg<1>( new_ );
    expect_bind_tx( last, new_ );
    expect_unbind_tx( new_, last );
    mock.EXPECT__gl_TexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    mock.EXPECT__gl_TexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    mock.EXPECT__gl_TexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    mock.EXPECT__gl_TexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  };

  gen_tx( 41, 42 );

  // Set texture image.
  expect_bind_tx( 41, 42 );
  expect_unbind_tx( 42, 41 );

  mock.EXPECT__gl_TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 64, 32,
                              0, GL_RGBA, GL_UNSIGNED_BYTE,
                              Not( Null() ) );

  // Set the u_atlas_size texture.
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 89, 64, 32 );

  // We bind the atlas texture on construction of the renderer
  // one final time. The corresponding unbind must come at the
  // end of this test case otherwise it interferes with other ex-
  // pect calls that we need to make in the mean time.
  expect_bind_tx( 41, 42 );

  // Create the offscreen texture (empty in initializer list).
  gen_tx( 42, 75 );

  // Create a new framebuffer.
  mock.EXPECT__gl_GenFramebuffers( 1, Not( Null() ) )
      .sets_arg<1>( 65 );
  mock.EXPECT__gl_DeleteFramebuffers( 1, Pointee( 65 ) );

  // Generate a new texture for the framebuffer.
  gen_tx( 42, 76 );

  mock.EXPECT__gl_DeleteTextures( 1, Pointee( 75 ) );
  mock.EXPECT__gl_DeleteTextures( 1, Pointee( 76 ) );
  mock.EXPECT__gl_DeleteTextures( 1, Pointee( 42 ) );

  // Set empty texture image.
  expect_bind_tx( 42, 76 );
  expect_unbind_tx( 76, 42 );
  mock.EXPECT__gl_TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 500,
                              400, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                              Null() );

  // Call set_color_attachment on the framebuffer.
  expect_bind_fb( 65, 65 );
  expect_unbind_fb( 65, 65 );
  mock.EXPECT__gl_FramebufferTexture2D( GL_FRAMEBUFFER,
                                        GL_COLOR_ATTACHMENT0,
                                        GL_TEXTURE_2D, 76, 0 );

  // Call is_framebuffer_complete on the framebuffer.
  expect_bind_fb( 65, 65 );
  expect_unbind_fb( 65, 65 );
  mock.EXPECT__gl_CheckFramebufferStatus( GL_FRAMEBUFFER )
      .returns( GL_FRAMEBUFFER_COMPLETE );

  // *** The test.

  vector<SpriteSheetConfig> sprite_config{
    {
      .img_path = testing::data_dir() / "images/64w_x_32h.png",
      .sprite_size = gfx::size{ .w = 32, .h = 32 },
      .sprites =
          {
            { "water", gfx::point{ .x = 0, .y = 0 } },
            { "grass", gfx::point{ .x = 1, .y = 0 } },
          },
    },
  };
  vector<AsciiFontSheetConfig> font_config;

  RendererConfig config{
    .logical_screen_size = gfx::size{ .w = 500, .h = 400 },
    .max_atlas_size      = gfx::size{ .w = 64, .h = 32 },
    .sprite_sheets       = sprite_config,
    .font_sheets         = font_config,
  };
  unique_ptr<Renderer> renderer =
      Renderer::create( config, [] {} );

  // Try zapping.
  {
    VertexRange vertex_range{
      .buffer = e_render_buffer::landscape_annex,
      .start  = 6, // zap the second rect.
      .finish = 12 };
    auto popper = renderer->push_mods( []( RendererMods& mods ) {
      mods.buffer_mods.buffer = e_render_buffer::landscape_annex;
    } );
    rr::Painter painter = renderer->painter();
    painter.draw_solid_rect( gfx::rect{}, gfx::pixel{} );
    painter.draw_solid_rect( gfx::rect{}, gfx::pixel{} );
    // Should not upload since the buffer is dirty.
    renderer->zap( vertex_range );

    // Now lets remove the dirty status by rendering.
    {
      // Bind/unbind vertex buffer.
      mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 99 );
      mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 41 );
      mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 41 );
      mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 99 );
      mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 99 );

      // Upload the data.

      mock.EXPECT__gl_BufferData(
          GL_ARRAY_BUFFER, 12 * sizeof( GenericVertex ),
          Not( Null() ), GL_STATIC_DRAW );

      mock.EXPECT__gl_UseProgram( 9 );

      // Bind/unbind vertex array.
      mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 98 );
      mock.EXPECT__gl_BindVertexArray( 21 );
      mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 21 );
      mock.EXPECT__gl_BindVertexArray( 98 );
      mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 98 );

      mock.EXPECT__gl_DrawArrays( GL_TRIANGLES, 0, 12 );

      renderer->testing_only_render_buffer(
          e_render_buffer::landscape_annex );
    }

    {
      // Now zap again that the buffer is not dirty and verify
      // that the sub data is re-uploaded.
      // Bind/unbind vertex buffer.
      mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 99 );
      mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 41 );
      mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 41 );
      mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 99 );
      mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                   Not( Null() ) )
          .sets_arg<1>( 99 );

      // Upload the sub section of data.
      mock.EXPECT__gl_BufferSubData(
          GL_ARRAY_BUFFER, 6 * sizeof( GenericVertex ),
          6 * sizeof( GenericVertex ), Not( Null() ) );
      renderer->zap( vertex_range );
    }
  }

  expect_unbind_tx( 42, 41 );
}

} // namespace
} // namespace rr
