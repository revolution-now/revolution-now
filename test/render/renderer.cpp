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

// render
#include "render/vertex.hpp"

// gl
#include "src/gl/iface-mock.hpp"
#include "src/gl/shader.hpp"

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
    { GL_INT, "in_type", true },                        //
    { GL_INT, "in_visible", true },                     //
    { GL_FLOAT, "in_depixelate", false },               //
    { GL_FLOAT_VEC2, "in_position", false },            //
    { GL_FLOAT_VEC2, "in_atlas_position", false },      //
    { GL_FLOAT_VEC2, "in_atlas_target_offset", false }, //
    { GL_FLOAT_VEC4, "in_fixed_color", false },         //
    { GL_FLOAT, "in_alpha_multiplier", false },         //
    { GL_FLOAT, "in_scaling", false },                  //
    { GL_FLOAT_VEC2, "in_translation", false },         //
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[render/renderer] workflows" ) {
  gl::MockOpenGL mock;

  int const num_get_errors = //
      56                     //
      + kExpectedAttributes.size();

  EXPECT_CALL( mock, gl_GetError() )
      .times( num_get_errors )
      .returns( GL_NO_ERROR );

  // Create vertex shader.
  EXPECT_CALL( mock, gl_CreateShader( GL_VERTEX_SHADER ) )
      .returns( 5 );
  EXPECT_CALL( mock,
               gl_ShaderSource(
                   5, 1, Pointee( StrContains( "gl_Position" ) ),
                   nullptr ) );
  EXPECT_CALL( mock, gl_CompileShader( 5 ) );
  EXPECT_CALL( mock, gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                                     Not( Null() ) ) )
      .sets_arg<2>( 1 );

  // Create fragment shader.
  EXPECT_CALL( mock, gl_CreateShader( GL_FRAGMENT_SHADER ) )
      .returns( 6 );
  EXPECT_CALL( mock,
               gl_ShaderSource(
                   6, 1, Pointee( StrContains( "final_color" ) ),
                   nullptr ) );
  EXPECT_CALL( mock, gl_CompileShader( 6 ) );
  EXPECT_CALL( mock, gl_GetShaderiv( 6, GL_COMPILE_STATUS,
                                     Not( Null() ) ) )
      .sets_arg<2>( 1 );

  // Delete the shaders.
  EXPECT_CALL( mock, gl_DeleteShader( 6 ) );
  EXPECT_CALL( mock, gl_DeleteShader( 5 ) );

  // Create shader program.

  // Create ProgramNonTyped.
  EXPECT_CALL( mock, gl_CreateProgram() ).returns( 9 );

  EXPECT_CALL( mock, gl_AttachShader( 9, 5 ) );
  EXPECT_CALL( mock, gl_AttachShader( 9, 6 ) );
  EXPECT_CALL( mock, gl_LinkProgram( 9 ) );
  EXPECT_CALL(
      mock, gl_GetProgramiv( 9, GL_LINK_STATUS, Not( Null() ) ) )
      .sets_arg<2>( 1 );
  EXPECT_CALL( mock, gl_ValidateProgram( 9 ) );
  EXPECT_CALL( mock, gl_GetProgramiv( 9, GL_VALIDATE_STATUS,
                                      Not( Null() ) ) )
      .sets_arg<2>( GL_TRUE );
  EXPECT_CALL( mock, gl_GetProgramInfoLog( 9, 512, Not( Null() ),
                                           Not( Null() ) ) )
      .sets_arg<2>( 0 );
  EXPECT_CALL( mock, gl_DetachShader( 9, 6 ) );
  EXPECT_CALL( mock, gl_DetachShader( 9, 5 ) );

  // Create uniforms.
  EXPECT_CALL(
      mock, gl_GetUniformLocation( 9, Eq<string>( "u_atlas" ) ) )
      .returns( 88 );
  EXPECT_CALL( mock, gl_GetUniformLocation(
                         9, Eq<string>( "u_atlas_size" ) ) )
      .returns( 89 );
  EXPECT_CALL( mock, gl_GetUniformLocation(
                         9, Eq<string>( "u_screen_size" ) ) )
      .returns( 90 );

  // Validate the program.
  EXPECT_CALL( mock, gl_GetProgramiv( 9, GL_ACTIVE_ATTRIBUTES,
                                      Not( Null() ) ) )
      .sets_arg<2>( kExpectedAttributes.size() );
  int idx = 0;
  for( auto& [type, name, is_int] : kExpectedAttributes ) {
    EXPECT_CALL( mock, gl_GetError() )
        .times( 2 )
        .returns( GL_NO_ERROR );
    string name_w_zero = name;
    name_w_zero.push_back( '\0' );
    EXPECT_CALL(
        mock, gl_GetActiveAttrib( 9, idx, 256, Not( Null() ),
                                  Not( Null() ), Not( Null() ),
                                  Not( Null() ) ) )
        .sets_arg<3>( name.size() + 1 )
        .sets_arg<4>( 0 ) // size, unused
        .sets_arg<5>( type )
        .sets_arg_array<6>( name_w_zero );
    EXPECT_CALL( mock, gl_GetAttribLocation(
                           9, Eq<string>( string( name ) ) ) )
        .returns( idx );
    ++idx;
  }

  // Release the program.
  EXPECT_CALL( mock, gl_DeleteProgram( 9 ) );

  // Try setting the uniforms to check their type.
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform1i( 88, 0 ) ); // u_atlas
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock,
               gl_Uniform2f( 89, 0.0, 0.0 ) ); // u_atlas_size
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock,
               gl_Uniform2f( 90, 0.0, 0.0 ) ); // u_screen_size

  // Set the u_atlas texture to zero.
  // NOTE: this is omitted even though the renderer does it be-
  // cause we are setting the same value that was already set
  // above and so the cached value is used.
  // EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  // EXPECT_CALL( mock, gl_Uniform1i( 88, 0 ) );

  // Set the u_screen_size texture.
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform2f( 90, 500.0, 400.0 ) );

  // Create the atlas texture.
  auto expect_bind = [&] {
    EXPECT_CALL( mock, gl_GetError() )
        .times( 5 )
        .returns( GL_NO_ERROR );
    EXPECT_CALL( mock, gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                       Not( Null() ) ) )
        .sets_arg<1>( 41 );
    EXPECT_CALL( mock, gl_BindTexture( GL_TEXTURE_2D, 42 ) );
    EXPECT_CALL( mock, gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                       Not( Null() ) ) )
        .sets_arg<1>( 42 );
    EXPECT_CALL( mock, gl_BindTexture( GL_TEXTURE_2D, 41 ) );
    EXPECT_CALL( mock, gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                       Not( Null() ) ) )
        .sets_arg<1>( 41 );
  };

  EXPECT_CALL( mock, gl_GenTextures( 1, Not( Null() ) ) )
      .sets_arg<1>( 42 );
  expect_bind();
  EXPECT_CALL( mock, gl_TexParameteri( GL_TEXTURE_2D,
                                       GL_TEXTURE_MIN_FILTER,
                                       GL_NEAREST ) );
  EXPECT_CALL( mock, gl_TexParameteri( GL_TEXTURE_2D,
                                       GL_TEXTURE_MAG_FILTER,
                                       GL_NEAREST ) );
  EXPECT_CALL(
      mock, gl_TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE ) );
  EXPECT_CALL(
      mock, gl_TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE ) );
  EXPECT_CALL( mock, gl_DeleteTextures( 1, Pointee( 42 ) ) );

  // Set texture image.
  expect_bind();
  EXPECT_CALL(
      mock, gl_TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, 0,
                           GL_RGBA, GL_UNSIGNED_BYTE,
                           Not( Null() ) ) );

  // Set the u_atlas_size texture.
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform2f( 89, 64, 32 ) );

  // Construct VertexArrayNonTyped.
  EXPECT_CALL( mock, gl_GenVertexArrays( 1, Not( Null() ) ) )
      .sets_arg<1>( 21 );

  // Construct vertex buffer.
  EXPECT_CALL( mock, gl_GenBuffers( 1, Not( Null() ) ) )
      .sets_arg<1>( 41 );

  // Bind vertex array.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );
  EXPECT_CALL( mock, gl_BindVertexArray( 21 ) );

  // Bind vertex buffer.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 40 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 41 ) );

  // Call to get max allowed attributes.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_MAX_VERTEX_ATTRIBS,
                                     Not( Null() ) ) )
      .sets_arg<1>( 100 )
      .times( kExpectedAttributes.size() );

  int i = 0;
  for( auto& [type, name, is_int] : kExpectedAttributes ) {
    EXPECT_CALL( mock, gl_GetError() )
        .times( 2 )
        .returns( GL_NO_ERROR );
    // Register attribute i.
    if( is_int ) {
      EXPECT_CALL( mock, gl_VertexAttribIPointer(
                             /*index=*/i, /*size=*/_,
                             /*type=*/_, /*stride=*/
                             sizeof( GenericVertex ),
                             /*pointer=*/_ ) );
    } else {
      // Register attribute i.
      EXPECT_CALL( mock, gl_VertexAttribPointer(
                             /*index=*/i, /*size=*/_, /*type=*/_,
                             /*normalized=*/false, /*stride=*/
                             sizeof( GenericVertex ),
                             /*pointer=*/_ ) );
    }
    EXPECT_CALL( mock, gl_EnableVertexAttribArray( i ) );
    ++i;
  }

  // Unbind vertex buffer.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 41 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 40 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 40 );

  // Unbind vertex array.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 21 );
  EXPECT_CALL( mock, gl_BindVertexArray( 20 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );

  // We bind the atlas texture on construction of the renderer
  // one final time.
  expect_bind();

  // Some more cleanup.
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 41 ) ) );
  EXPECT_CALL( mock, gl_DeleteVertexArrays( 1, Pointee( 21 ) ) );

  // *** The test.

  vector<SpriteSheetConfig> sprite_config{
      {
          .img_path =
              testing::data_dir() / "images/64w_x_32h.png",
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
}

} // namespace
} // namespace rr
