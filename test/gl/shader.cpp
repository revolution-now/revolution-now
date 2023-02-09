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

// Testing
#include "test/mocks/gl/iface.hpp"

// refl
#include "refl/ext.hpp"

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
};

} // namespace
} // namespace gl

namespace refl {

// Reflection info for struct Scale.
template<>
struct traits<gl::Vertex> {
  using type = gl::Vertex;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name = "Vertex";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "some_vec3", &gl::Vertex::some_vec3,
                         offsetof( type, some_vec3 ) },
      refl::StructField{ "some_float", &gl::Vertex::some_float,
                         offsetof( type, some_float ) },
  };
};

} // namespace refl

namespace gl {
namespace {

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

  mock.EXPECT__gl_GetError().times( 5 ).returns( GL_NO_ERROR );

  mock.EXPECT__gl_CreateShader( GL_VERTEX_SHADER ).returns( 5 );

  mock.EXPECT__gl_ShaderSource(
      5, 1, Pointee( Eq<string>( "my shader source" ) ),
      nullptr );
  mock.EXPECT__gl_CompileShader( 5 );
  mock.EXPECT__gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  auto vert_shader = Shader::create( e_shader_type::vertex,
                                     "my shader source" );
  REQUIRE( vert_shader.has_value() );
  REQUIRE( vert_shader->id() == 5 );

  mock.EXPECT__gl_DeleteShader( 5 );
}

TEST_CASE( "[shader] fragment shader create successful" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 5 ).returns( GL_NO_ERROR );

  mock.EXPECT__gl_CreateShader( GL_FRAGMENT_SHADER )
      .returns( 5 );

  mock.EXPECT__gl_ShaderSource(
      5, 1, Pointee( Eq<string>( "my shader source" ) ),
      nullptr );
  mock.EXPECT__gl_CompileShader( 5 );
  mock.EXPECT__gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  auto frag_shader = Shader::create( e_shader_type::fragment,
                                     "my shader source" );
  REQUIRE( frag_shader.has_value() );
  REQUIRE( frag_shader->id() == 5 );

  mock.EXPECT__gl_DeleteShader( 5 );
}

TEST_CASE( "[shader] vertex shader compile fails" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 5 ).returns( GL_NO_ERROR );

  mock.EXPECT__gl_CreateShader( GL_VERTEX_SHADER ).returns( 5 );

  mock.EXPECT__gl_ShaderSource(
      5, 1, Pointee( Eq<string>( "my shader source" ) ),
      nullptr );
  mock.EXPECT__gl_CompileShader( 5 );
  mock.EXPECT__gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 0 );
  mock.EXPECT__gl_GetShaderInfoLog( 5, 512, nullptr,
                                    Not( Null() ) )
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
  mock.EXPECT__gl_GetError().times( 59 ).returns( GL_NO_ERROR );

  mock.EXPECT__gl_CreateShader( GL_VERTEX_SHADER ).returns( 5 );

  mock.EXPECT__gl_ShaderSource(
      5, 1, Pointee( Eq<string>( "my shader source" ) ),
      nullptr );
  mock.EXPECT__gl_CompileShader( 5 );
  mock.EXPECT__gl_GetShaderiv( 5, GL_COMPILE_STATUS,
                               Not( Null() ) )
      .sets_arg<2>( 1 );

  // Create ProgramNonTyped.
  mock.EXPECT__gl_CreateProgram().returns( 9 );

  mock.EXPECT__gl_AttachShader( 9, 5 ).times( 2 );
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
  mock.EXPECT__gl_DetachShader( 9, 5 ).times( 2 );

  UNWRAP_CHECK( vert_shader,
                Shader::create( e_shader_type::vertex,
                                "my shader source" ) );
  REQUIRE( vert_shader.id() == 5 );

  // Create uniforms.
  mock.EXPECT__gl_GetUniformLocation( 9,
                                      Eq<string>( "some_vec2" ) )
      .returns( 88 );
  mock.EXPECT__gl_GetUniformLocation( 9,
                                      Eq<string>( "some_int" ) )
      .returns( 89 );

  // Validate the program.
  mock.EXPECT__gl_GetProgramiv( 9, GL_ACTIVE_ATTRIBUTES,
                                Not( Null() ) )
      .sets_arg<2>( 2 );
  mock.EXPECT__gl_GetActiveAttrib( 9, 0, 256, Not( Null() ),
                                   Not( Null() ), Not( Null() ),
                                   Not( Null() ) )
      .sets_arg<3>( string( "some_vec3" ).size() + 1 )
      .sets_arg<4>( 0 ) // size, unused
      .sets_arg<5>( GL_FLOAT_VEC3 )
      .sets_arg_array<6>( "some_vec3\0"s );
  mock.EXPECT__gl_GetAttribLocation( 9,
                                     Eq<string>( "some_vec3" ) )
      .returns( 0 );
  mock.EXPECT__gl_GetActiveAttrib( 9, 1, 256, Not( Null() ),
                                   Not( Null() ), Not( Null() ),
                                   Not( Null() ) )
      .sets_arg<3>( string( "some_float" ).size() + 1 )
      .sets_arg<4>( 0 ) // size, unused
      .sets_arg<5>( GL_FLOAT )
      .sets_arg_array<6>( "some_float\0"s );
  mock.EXPECT__gl_GetAttribLocation( 9,
                                     Eq<string>( "some_float" ) )
      .returns( 1 );

  // Try setting the uniforms to check their type.
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 88, 0, 0 );
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform1i( 89, 0 );

  // Since we're just mocking we can use the same shader twice.
  auto maybe_pgrm =
      ProgramType::create( vert_shader, vert_shader );
  REQUIRE( maybe_pgrm.has_value() );
  auto& pgrm = *maybe_pgrm;
  REQUIRE( pgrm.id() == 9 );

  // Use the program.
  mock.EXPECT__gl_UseProgram( 9 );
  pgrm.use();

  // Construct VertexArrayNonTyped.
  mock.EXPECT__gl_GenVertexArrays( 1, Not( Null() ) )
      .sets_arg<1>( 21 );

  // Construct vertex buffer.
  mock.EXPECT__gl_GenBuffers( 1, Not( Null() ) )
      .sets_arg<1>( 41 );

  // Bind vertex array.
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 20 );
  mock.EXPECT__gl_BindVertexArray( 21 );

  // Bind vertex buffer.
  mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 40 );
  mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 41 );

  // Call to get max allowed attributes.
  mock.EXPECT__gl_GetIntegerv( GL_MAX_VERTEX_ATTRIBS,
                               Not( Null() ) )
      .sets_arg<1>( 10 )
      .times( 2 );

  // Register attribute 0 (vec3).
  mock.EXPECT__gl_VertexAttribPointer(
      /*index=*/0, /*size=*/3, /*type=*/GL_FLOAT,
      /*normalized=*/false, /*stride=*/
      sizeof( Vertex ),
      /*pointer=*/nullptr );
  mock.EXPECT__gl_EnableVertexAttribArray( 0 );

  // Register attribute 1 (float).

  mock.EXPECT__gl_VertexAttribPointer(
      /*index=*/1, /*size=*/1, /*type=*/GL_FLOAT,
      /*normalized=*/false, /*stride=*/sizeof( Vertex ),
      /*pointer=*/(void*)offsetof( Vertex, some_float ) );
  mock.EXPECT__gl_EnableVertexAttribArray( 1 );

  // Unbind vertex buffer.
  mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 41 );
  mock.EXPECT__gl_BindBuffer( GL_ARRAY_BUFFER, 40 );
  mock.EXPECT__gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 40 );

  // Unbind vertex array.
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 21 );
  mock.EXPECT__gl_BindVertexArray( 20 );
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 20 );

  VertexArray<VertexBuffer<Vertex>> vertex_array;

  // Run the program.
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 20 );
  mock.EXPECT__gl_BindVertexArray( 21 );
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_DrawArrays( GL_TRIANGLES, 0, 99 );
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 21 );
  mock.EXPECT__gl_BindVertexArray( 20 );
  mock.EXPECT__gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                               Not( Null() ) )
      .sets_arg<1>( 20 );
  pgrm.run( vertex_array, 99 );

  // Set some uniforms.
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform2f( 88, 3.4, 4.5 );
  using namespace ::base::literals;
  pgrm["some_vec2"_t] = { .x = 3.4, .y = 4.5 };
  mock.EXPECT__gl_UseProgram( 9 );
  mock.EXPECT__gl_Uniform1i( 89, 888 );
  pgrm["some_int"_t] = 888;

  // Cleanup.
  mock.EXPECT__gl_DeleteBuffers( 1, Pointee( 41 ) );
  mock.EXPECT__gl_DeleteVertexArrays( 1, Pointee( 21 ) );
  mock.EXPECT__gl_DeleteProgram( 9 );
  mock.EXPECT__gl_DeleteShader( 5 );
}

} // namespace
} // namespace gl
