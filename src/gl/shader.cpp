/****************************************************************
**shader.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-30.
*
* Description: OpenGL Shader Wrappers.
*
*****************************************************************/
#include "shader.hpp"

// gl
#include "error.hpp"

// Glad
#include "glad/glad.h"

using namespace std;

namespace gl {

/****************************************************************
** Shader Type
*****************************************************************/
void to_str( e_shader_type type, std::string& out ) {
  switch( type ) {
    case e_shader_type::vertex: out += "vertex"; return;
    case e_shader_type::fragment: out += "fragment"; return;
  }
}

/****************************************************************
** Vertex/Fragment Shader
*****************************************************************/
Shader::Shader( ObjId id ) : base::zero<Shader, ObjId>( id ) {}

base::expect<Shader, std::string> Shader::create(
    e_shader_type type, std::string const& code ) {
  GLenum gl_shader_type = 0;
  switch( type ) {
    case e_shader_type::vertex:
      gl_shader_type = GL_VERTEX_SHADER;
      break;
    case e_shader_type::fragment:
      gl_shader_type = GL_FRAGMENT_SHADER;
      break;
  }
  GLuint      id = GL_CHECK( glCreateShader( gl_shader_type ) );
  char const* p_code = code.c_str();
  // According to the docs, the fact that we put nullptr for the
  // last argument requires that the string be null terminated.
  GL_CHECK( glShaderSource( id, 1, &p_code, nullptr ) );
  GL_CHECK( glCompileShader( id ) );
  // Check for compiler errors.
  int              success;
  constexpr size_t kErrorLength = 512;
  char             errors[kErrorLength];
  GL_CHECK( glGetShaderiv( id, GL_COMPILE_STATUS, &success ) );
  if( !success ) {
    GL_CHECK(
        glGetShaderInfoLog( id, kErrorLength, NULL, errors ) );
    return fmt::format( "{} shader compilation failed: {}",
                        base::to_str( type ), errors );
  }
  return Shader( id );
}

Shader::attacher::attacher( Program const& pgrm,
                            Shader const&  shader )
  : pgrm_( pgrm ), shader_( shader ) {
  GL_CHECK( glAttachShader( pgrm_.id(), shader_.id() ) );
}

Shader::attacher::~attacher() {
  GL_CHECK( glDetachShader( pgrm_.id(), shader_.id() ) );
}

void Shader::free_resource() {
  ObjId shader_id = resource();
  DCHECK( shader_id != 0 );
  GL_CHECK( glDeleteShader( shader_id ) );
}

/****************************************************************
** Shader Program
*****************************************************************/
Program::Program( ObjId id )
  : base::zero<Program, ObjId>( id ) {}

base::expect<Program, std::string> Program::create(
    Shader const& vertex, Shader const& fragment ) {
  Program pgrm( GL_CHECK( glCreateProgram() ) );

  // Put this in a block so that se can detach the shaders before
  // moving out of the program object to return it.
  {
    auto vert_attacher = vertex.attach( pgrm );
    auto frag_attacher = fragment.attach( pgrm );

    GL_CHECK( glLinkProgram( pgrm.id() ) );
    // Check for linking errors.
    int              success;
    constexpr size_t kErrorLength = 512;
    char             errors[kErrorLength];
    GL_CHECK(
        glGetProgramiv( pgrm.id(), GL_LINK_STATUS, &success ) );
    if( !success ) {
      GL_CHECK( glGetProgramInfoLog( pgrm.id(), kErrorLength,
                                     NULL, errors ) );
      return fmt::format( "shader program linking failed: {}",
                          errors );
    }

    GL_CHECK( glValidateProgram( pgrm.id() ) );
    GLint validation_successful;
    GL_CHECK( glGetProgramiv( pgrm.id(), GL_VALIDATE_STATUS,
                              &validation_successful ) );
    GLint out_size;
    GL_CHECK( glGetProgramInfoLog( pgrm.id(),
                                   /*maxlength=*/kErrorLength,
                                   &out_size, errors ) );
    string_view info_log( errors, out_size );
    if( validation_successful != GL_TRUE )
      return fmt::format(
          "shader program failed validation; info log: {}",
          info_log );
  }

  return pgrm;
}

void Program::use() const { GL_CHECK( glUseProgram( id() ) ); }

void Program::free_resource() {
  ObjId pgrm_id = resource();
  DCHECK( pgrm_id != 0 );
  GL_CHECK( glDeleteProgram( pgrm_id ) );
}

void Program::run( VertexArrayNonTyped const& vert_array,
                   int                        num_vertices ) {
  DCHECK( num_vertices >= 0 );
  use();
  auto binder = vert_array.bind();
  GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, num_vertices ) );
}

} // namespace gl
