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

// base
#include "base/error.hpp"

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

Shader::attacher::attacher( ProgramNonTyped const& pgrm,
                            Shader const&          shader )
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
** ProgramNonTyped
*****************************************************************/
ProgramNonTyped::ProgramNonTyped( ObjId id )
  : base::zero<ProgramNonTyped, ObjId>( id ) {}

base::expect<ProgramNonTyped, std::string>
ProgramNonTyped::create( Shader const& vertex,
                         Shader const& fragment ) {
  ProgramNonTyped pgrm( GL_CHECK( glCreateProgram() ) );

  // Put this in a block so that se can detach the shaders before
  // moving out of the ProgramNonTyped object to return it.
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
      return fmt::format(
          "shader ProgramNonTyped linking failed: {}", errors );
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
          "shader ProgramNonTyped failed validation; info log: "
          "{}",
          info_log );
  }

  return pgrm;
}

void ProgramNonTyped::use() const {
  GL_CHECK( glUseProgram( id() ) );
}

void ProgramNonTyped::free_resource() {
  ObjId pgrm_id = resource();
  DCHECK( pgrm_id != 0 );
  GL_CHECK( glDeleteProgram( pgrm_id ) );
}

void ProgramNonTyped::run( VertexArrayNonTyped const& vert_array,
                           int num_vertices ) const {
  DCHECK( num_vertices >= 0 );
  use();
  auto binder = vert_array.bind();
  GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, num_vertices ) );
}

int ProgramNonTyped::num_input_attribs() const {
  int num_active;
  GL_CHECK( glGetProgramiv( id(), GL_ACTIVE_ATTRIBUTES,
                            &num_active ) );
  return num_active;
}

base::expect<e_attrib_compound_type, string>
ProgramNonTyped::attrib_compound_type( int idx ) const {
  GLint  size;
  GLenum type;
  // Need this in case it wants to write a null terminator to the
  // name against our wishes.
  constexpr size_t kBufSize = 256;
  char             c_name[kBufSize];
  GLsizei          name_length;
  GL_CHECK( glGetActiveAttrib(
      /*program=*/id(),
      /*index=*/idx,
      /*bufsize=*/kBufSize,
      /*length=*/&name_length, // don't write name
      /*size=*/&size,
      /*type=*/&type,
      /*name=*/c_name ) );
  GLint location =
      GL_CHECK( glGetAttribLocation( id(), c_name ) );
  if( location != idx )
    return fmt::format(
        "shader program vertex attribute at index {} (\"{}\") "
        "does not have matching location index, instead has {}.",
        idx, c_name, location );
  return from_GL( type );
}

} // namespace gl
