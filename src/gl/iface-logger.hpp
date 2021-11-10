/****************************************************************
**iface-logger.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Implementation of IOpenGL that logs and forwards.
*
*****************************************************************/
#pragma once

// gl
#include "iface.hpp"

// C++ standard library
#include <type_traits>

namespace gl {

struct OpenGLWithLogger : IOpenGL {
private:
  bool     logging_enabled_ = false;
  IOpenGL* next_            = nullptr;

public:
  void enable_logging( bool enabled );

public:
  OpenGLWithLogger( IOpenGL* next ) : next_( next ) {}

  void gl_AttachShader( GLuint program, GLuint shader ) override;

  void gl_BindBuffer( GLenum target, GLuint buffer ) override;

  void gl_BindVertexArray( GLuint array ) override;

  void gl_BufferData( GLenum target, GLsizeiptr size,
                      void const* data, GLenum usage ) override;

  void gl_BufferSubData( GLenum target, GLintptr offset,
                         GLsizeiptr  size,
                         void const* data ) override;

  void gl_CompileShader( GLuint shader ) override;

  GLuint gl_CreateProgram() override;

  GLuint gl_CreateShader( GLenum type ) override;

  void gl_DeleteBuffers( GLsizei       n,
                         GLuint const* buffers ) override;

  void gl_DeleteProgram( GLuint program ) override;

  void gl_DeleteShader( GLuint shader ) override;

  void gl_DeleteVertexArrays( GLsizei       n,
                              GLuint const* arrays ) override;

  void gl_DetachShader( GLuint program, GLuint shader ) override;

  void gl_DrawArrays( GLenum mode, GLint first,
                      GLsizei count ) override;

  void gl_EnableVertexAttribArray( GLuint index ) override;

  void gl_GenBuffers( GLsizei n, GLuint* buffers ) override;

  void gl_GenVertexArrays( GLsizei n, GLuint* arrays ) override;

  void gl_GetActiveAttrib( GLuint program, GLuint index,
                           GLsizei bufSize, GLsizei* length,
                           GLint* size, GLenum* type,
                           GLchar* name ) override;

  GLint gl_GetAttribLocation( GLuint        program,
                              GLchar const* name ) override;

  GLenum gl_GetError() override;

  void gl_GetIntegerv( GLenum pname, GLint* data ) override;

  void gl_GetProgramInfoLog( GLuint program, GLsizei bufSize,
                             GLsizei* length,
                             GLchar*  infoLog ) override;

  void gl_GetProgramiv( GLuint program, GLenum pname,
                        GLint* params ) override;

  void gl_GetShaderInfoLog( GLuint shader, GLsizei bufSize,
                            GLsizei* length,
                            GLchar*  infoLog ) override;

  void gl_GetShaderiv( GLuint shader, GLenum pname,
                       GLint* params ) override;

  GLint gl_GetUniformLocation( GLuint        program,
                               GLchar const* name ) override;

  void gl_LinkProgram( GLuint program ) override;

  void gl_ShaderSource( GLuint shader, GLsizei count,
                        GLchar const* const* string,
                        GLint const*         length ) override;

  void gl_Uniform1f( GLint location, GLfloat v0 ) override;

  void gl_Uniform1i( GLint location, GLint v0 ) override;

  void gl_Uniform2f( GLint location, GLfloat v0,
                     GLfloat v1 ) override;

  void gl_UseProgram( GLuint program ) override;

  void gl_ValidateProgram( GLuint program ) override;

  void gl_VertexAttribPointer( GLuint index, GLint size,
                               GLenum type, GLboolean normalized,
                               GLsizei     stride,
                               void const* pointer ) override;
};

static_assert( !std::is_abstract_v<OpenGLWithLogger> );

} // namespace gl
