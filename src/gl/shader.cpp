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
#include "iface.hpp"

// base
#include "base/error.hpp"

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
  GLuint id =
      GL_CHECK( CALL_GL( gl_CreateShader, gl_shader_type ) );
  char const* p_code = code.c_str();
  // According to the docs, the fact that we put nullptr for the
  // last argument requires that the string be null terminated.
  GL_CHECK(
      CALL_GL( gl_ShaderSource, id, 1, &p_code, nullptr ) );
  GL_CHECK( CALL_GL( gl_CompileShader, id ) );
  // Check for compiler errors.
  int              success;
  constexpr size_t kErrorLength = 512;
  char             errors[kErrorLength];
  GL_CHECK( CALL_GL( gl_GetShaderiv, id, GL_COMPILE_STATUS,
                     &success ) );
  if( !success ) {
    GL_CHECK( CALL_GL( gl_GetShaderInfoLog, id, kErrorLength,
                       NULL, errors ) );
    return fmt::format( "{} shader compilation failed: {}",
                        base::to_str( type ), errors );
  }
  return Shader( id );
}

Shader::attacher::attacher( ProgramNonTyped const& pgrm,
                            Shader const&          shader )
  : pgrm_( pgrm ), shader_( shader ) {
  GL_CHECK(
      CALL_GL( gl_AttachShader, pgrm_.id(), shader_.id() ) );
}

Shader::attacher::~attacher() {
  GL_CHECK(
      CALL_GL( gl_DetachShader, pgrm_.id(), shader_.id() ) );
}

void Shader::free_resource() {
  ObjId shader_id = resource();
  DCHECK( shader_id != 0 );
  GL_CHECK( CALL_GL( gl_DeleteShader, shader_id ) );
}

/****************************************************************
** ProgramNonTyped
*****************************************************************/
ProgramNonTyped::ProgramNonTyped( ObjId id )
  : base::zero<ProgramNonTyped, ObjId>( id ) {}

base::expect<ProgramNonTyped, std::string>
ProgramNonTyped::create( Shader const& vertex,
                         Shader const& fragment ) {
  ProgramNonTyped pgrm(
      GL_CHECK( CALL_GL( gl_CreateProgram ) ) );

  // Put this in a block so that se can detach the shaders before
  // moving out of the ProgramNonTyped object to return it.
  {
    auto vert_attacher = vertex.attach( pgrm );
    auto frag_attacher = fragment.attach( pgrm );

    GL_CHECK( CALL_GL( gl_LinkProgram, pgrm.id() ) );
    // Check for linking errors.
    int              success;
    constexpr size_t kErrorLength = 512;
    char             errors[kErrorLength];
    GL_CHECK( CALL_GL( gl_GetProgramiv, pgrm.id(),
                       GL_LINK_STATUS, &success ) );
    if( !success ) {
      GL_CHECK( CALL_GL( gl_GetProgramInfoLog, pgrm.id(),
                         kErrorLength, NULL, errors ) );
      return fmt::format(
          "shader ProgramNonTyped linking failed: {}", errors );
    }

    GL_CHECK( CALL_GL( gl_ValidateProgram, pgrm.id() ) );
    GLint validation_successful;
    GL_CHECK( CALL_GL( gl_GetProgramiv, pgrm.id(),
                       GL_VALIDATE_STATUS,
                       &validation_successful ) );
    GLint out_size;
    GL_CHECK( CALL_GL( gl_GetProgramInfoLog, pgrm.id(),
                       /*maxlength=*/kErrorLength, &out_size,
                       errors ) );
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
  GL_CHECK( CALL_GL( gl_UseProgram, id() ) );
}

void ProgramNonTyped::free_resource() {
  ObjId pgrm_id = resource();
  DCHECK( pgrm_id != 0 );
  GL_CHECK( CALL_GL( gl_DeleteProgram, pgrm_id ) );
}

void ProgramNonTyped::run( VertexArrayNonTyped const& vert_array,
                           int num_vertices ) const {
  DCHECK( num_vertices >= 0 );
  use();
  auto binder = vert_array.bind();
  GL_CHECK(
      CALL_GL( gl_DrawArrays, GL_TRIANGLES, 0, num_vertices ) );
}

int ProgramNonTyped::num_input_attribs() const {
  int num_active;
  GL_CHECK( CALL_GL( gl_GetProgramiv, id(), GL_ACTIVE_ATTRIBUTES,
                     &num_active ) );
  return num_active;
}

base::expect<pair<e_attrib_compound_type, int /*location*/>,
             string>
ProgramNonTyped::attrib_compound_type( int idx ) const {
  GLint  size;
  GLenum type;
  // Need this in case it wants to write a null terminator to the
  // name against our wishes.
  constexpr size_t kBufSize = 256;
  char             c_name[kBufSize];
  GLsizei          name_length;
  GL_CHECK( CALL_GL( gl_GetActiveAttrib,
                     /*program=*/id(),
                     /*index=*/idx,
                     /*bufsize=*/kBufSize,
                     /*length=*/&name_length,
                     /*size=*/&size,
                     /*type=*/&type,
                     /*name=*/c_name ) );
  GLint location =
      GL_CHECK( CALL_GL( gl_GetAttribLocation, id(), c_name ) );
  return pair{ from_GL( type ), location };
}

} // namespace gl
