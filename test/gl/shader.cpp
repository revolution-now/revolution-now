/****************************************************************
**shader.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-02.
*
* Description: Unit tests for the src/gl/shader.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/shader.hpp"

// gl
#include "src/gl/iface-mock.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;
using namespace ::mock::matchers;

using ::base::unexpected;

struct Vertex {
  vec3  some_vec3;
  float some_float;

  static consteval auto attributes() {
    return tuple{ VERTEX_ATTRIB_HOLDER( Vertex, some_vec3 ),
                  VERTEX_ATTRIB_HOLDER( Vertex, some_float ) };
  }
};

using ProgramAttributes = mp::list<gl::vec3, float>;

struct ProgramUniforms {
  static constexpr tuple uniforms{
      gl::UniformSpec<gl::vec2>( "some_vec2" ),
      gl::UniformSpec<int>( "some_int" ),
  };
};

using ProgramType =
    gl::Program<ProgramAttributes, ProgramUniforms>;

TEST_CASE( "[shader] vertex shader create successful" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 5 )
      .returns( GL_NO_ERROR );

  EXPECT_CALL( mock, gl_CreateShader( GL_VERTEX_SHADER ) )
      .returns( 5 );
  EXPECT_CALL(
      mock,
      gl_ShaderSource(
          5, 1, Pointee( Eq<string>( "my shader source" ) ),
          nullptr ) );
  EXPECT_CALL( mock, gl_CompileShader( 5 ) );
  EXPECT_CALL( mock, gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                                     Not( Null() ) ) )
      .sets_arg<2>( 1 );

  auto vert_shader = Shader::create( e_shader_type::vertex,
                                     "my shader source" );
  REQUIRE( vert_shader.has_value() );
  REQUIRE( vert_shader->id() == 5 );

  EXPECT_CALL( mock, gl_DeleteShader( 5 ) );
}

TEST_CASE( "[shader] fragment shader create successful" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 5 )
      .returns( GL_NO_ERROR );

  EXPECT_CALL( mock, gl_CreateShader( GL_FRAGMENT_SHADER ) )
      .returns( 5 );
  EXPECT_CALL(
      mock,
      gl_ShaderSource(
          5, 1, Pointee( Eq<string>( "my shader source" ) ),
          nullptr ) );
  EXPECT_CALL( mock, gl_CompileShader( 5 ) );
  EXPECT_CALL( mock, gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                                     Not( Null() ) ) )
      .sets_arg<2>( 1 );

  auto frag_shader = Shader::create( e_shader_type::fragment,
                                     "my shader source" );
  REQUIRE( frag_shader.has_value() );
  REQUIRE( frag_shader->id() == 5 );

  EXPECT_CALL( mock, gl_DeleteShader( 5 ) );
}

TEST_CASE( "[shader] vertex shader compile fails" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 5 )
      .returns( GL_NO_ERROR );

  EXPECT_CALL( mock, gl_CreateShader( GL_VERTEX_SHADER ) )
      .returns( 5 );
  EXPECT_CALL(
      mock,
      gl_ShaderSource(
          5, 1, Pointee( Eq<string>( "my shader source" ) ),
          nullptr ) );
  EXPECT_CALL( mock, gl_CompileShader( 5 ) );
  EXPECT_CALL( mock, gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                                     Not( Null() ) ) )
      .sets_arg<2>( 0 );
  EXPECT_CALL( mock, gl_GetShaderInfoLog( 5, 512, nullptr,
                                          Not( Null() ) ) )
      .sets_arg_array<3>( "this is an error\0"s );

  auto vert_shader = Shader::create( e_shader_type::vertex,
                                     "my shader source" );
  REQUIRE( vert_shader.has_error() );
  REQUIRE(
      vert_shader.error() ==
      "vertex shader compilation failed: this is an error"s );
}

TEST_CASE( "[shader] Program" ) {
  gl::MockOpenGL mock;

  // Create shader.
  EXPECT_CALL( mock, gl_GetError() )
      .times( 59 )
      .returns( GL_NO_ERROR );

  EXPECT_CALL( mock, gl_CreateShader( GL_VERTEX_SHADER ) )
      .returns( 5 );
  EXPECT_CALL(
      mock,
      gl_ShaderSource(
          5, 1, Pointee( Eq<string>( "my shader source" ) ),
          nullptr ) );
  EXPECT_CALL( mock, gl_CompileShader( 5 ) );
  EXPECT_CALL( mock, gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                                     Not( Null() ) ) )
      .sets_arg<2>( 1 );

  // Create ProgramNonTyped.
  EXPECT_CALL( mock, gl_CreateProgram() ).returns( 9 );

  EXPECT_CALL( mock, gl_AttachShader( 9, 5 ) ).times( 2 );
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
  EXPECT_CALL( mock, gl_DetachShader( 9, 5 ) ).times( 2 );

  UNWRAP_CHECK( vert_shader,
                Shader::create( e_shader_type::vertex,
                                "my shader source" ) );
  REQUIRE( vert_shader.id() == 5 );

  // Create uniforms.
  EXPECT_CALL( mock, gl_GetUniformLocation(
                         9, Eq<string>( "some_vec2" ) ) )
      .returns( 88 );
  EXPECT_CALL( mock, gl_GetUniformLocation(
                         9, Eq<string>( "some_int" ) ) )
      .returns( 89 );

  // Validate the program.
  EXPECT_CALL( mock, gl_GetProgramiv( 9, GL_ACTIVE_ATTRIBUTES,
                                      Not( Null() ) ) )
      .sets_arg<2>( 2 );
  EXPECT_CALL( mock, gl_GetActiveAttrib(
                         9, 0, 256, Not( Null() ), Not( Null() ),
                         Not( Null() ), Not( Null() ) ) )
      .sets_arg<3>( string( "some_vec3" ).size() + 1 )
      .sets_arg<4>( 0 ) // size, unused
      .sets_arg<5>( GL_FLOAT_VEC3 )
      .sets_arg_array<6>( "some_vec3\0"s );
  EXPECT_CALL( mock, gl_GetAttribLocation(
                         9, Eq<string>( "some_vec3" ) ) )
      .returns( 0 );
  EXPECT_CALL( mock, gl_GetActiveAttrib(
                         9, 1, 256, Not( Null() ), Not( Null() ),
                         Not( Null() ), Not( Null() ) ) )
      .sets_arg<3>( string( "some_float" ).size() + 1 )
      .sets_arg<4>( 0 ) // size, unused
      .sets_arg<5>( GL_FLOAT )
      .sets_arg_array<6>( "some_float\0"s );
  EXPECT_CALL( mock, gl_GetAttribLocation(
                         9, Eq<string>( "some_float" ) ) )
      .returns( 1 );

  // Try setting the uniforms to check their type.
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform2f( 88, 0, 0 ) );
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform1i( 89, 0 ) );

  // Since we're just mocking we can use the same shader twice.
  auto pgrm = ProgramType::create( vert_shader, vert_shader );
  REQUIRE( pgrm.has_value() );
  REQUIRE( pgrm->id() == 9 );

  // Use the program.
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  pgrm->use();

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
      .sets_arg<1>( 10 )
      .times( 2 );

  // Register attribute 0 (vec3).
  EXPECT_CALL( mock,
               gl_VertexAttribPointer(
                   /*index=*/0, /*size=*/3, /*type=*/GL_FLOAT,
                   /*normalized=*/false, /*stride=*/
                   sizeof( Vertex ),
                   /*pointer=*/(void*)0 ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 0 ) );

  // Register attribute 1 (float).
  EXPECT_CALL(
      mock,
      gl_VertexAttribPointer(
          /*index=*/1, /*size=*/1, /*type=*/GL_FLOAT,
          /*normalized=*/false, /*stride=*/sizeof( Vertex ),
          /*pointer=*/(void*)offsetof( Vertex, some_float ) ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 1 ) );

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

  VertexArray<VertexBuffer<Vertex>> vertex_array;

  // Run the program.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );
  EXPECT_CALL( mock, gl_BindVertexArray( 21 ) );
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_DrawArrays( GL_TRIANGLES, 0, 99 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 21 );
  EXPECT_CALL( mock, gl_BindVertexArray( 20 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );
  pgrm->run( vertex_array, 99 );

  // Set some uniforms.
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform2f( 88, 3.4, 4.5 ) );
  pgrm->set_uniform<"some_vec2">( { .x = 3.4, .y = 4.5 } );
  EXPECT_CALL( mock, gl_UseProgram( 9 ) );
  EXPECT_CALL( mock, gl_Uniform1i( 89, 888 ) );
  pgrm->set_uniform<"some_int">( 888 );

  // Cleanup.
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 41 ) ) );
  EXPECT_CALL( mock, gl_DeleteVertexArrays( 1, Pointee( 21 ) ) );
  EXPECT_CALL( mock, gl_DeleteProgram( 9 ) );
  EXPECT_CALL( mock, gl_DeleteShader( 5 ) );
}

} // namespace
} // namespace gl
