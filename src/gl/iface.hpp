/****************************************************************
**iface.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Virtual interface for OpenGL methods.
*
*****************************************************************/
#pragma once

// Glad
#include "glad/glad.h"

// base-util
#include "base-util/pp.hpp"

#define CALL_GL( ... ) \
  PP_ONE_OR_MORE_ARGS( CALL_GL, __VA_ARGS__ )

#define CALL_GL_MULTI( name, ... ) \
  ::gl::global_gl_implementation()->name( __VA_ARGS__ );

#define CALL_GL_SINGLE( name ) \
  ::gl::global_gl_implementation()->name();

namespace gl {

// In this interface we avoid using any tokens whose names coin-
// cide with the names of OpenGL functions because some OpenGL
// loader libraries (such as Glad) define those as macros, hence
// the underscore after the gl.
struct IOpenGL {
  virtual ~IOpenGL() = default;

  virtual void gl_AttachShader( GLuint program,
                                GLuint shader ) = 0;

  virtual void gl_BindBuffer( GLenum target, GLuint buffer ) = 0;

  virtual void gl_BindVertexArray( GLuint array ) = 0;

  virtual void gl_BufferData( GLenum target, GLsizeiptr size,
                              void const* data,
                              GLenum      usage ) = 0;

  virtual void gl_BufferSubData( GLenum target, GLintptr offset,
                                 GLsizeiptr  size,
                                 void const* data ) = 0;

  virtual void gl_CompileShader( GLuint shader ) = 0;

  virtual GLuint gl_CreateProgram() = 0;

  virtual GLuint gl_CreateShader( GLenum type ) = 0;

  virtual void gl_DeleteBuffers( GLsizei       n,
                                 GLuint const* buffers ) = 0;

  virtual void gl_DeleteProgram( GLuint program ) = 0;

  virtual void gl_DeleteShader( GLuint shader ) = 0;

  virtual void gl_DeleteVertexArrays( GLsizei       n,
                                      GLuint const* arrays ) = 0;

  virtual void gl_DetachShader( GLuint program,
                                GLuint shader ) = 0;

  virtual void gl_DrawArrays( GLenum mode, GLint first,
                              GLsizei count ) = 0;

  virtual void gl_EnableVertexAttribArray( GLuint index ) = 0;

  virtual void gl_GenBuffers( GLsizei n, GLuint* buffers ) = 0;

  virtual void gl_GenVertexArrays( GLsizei n,
                                   GLuint* arrays ) = 0;

  virtual void gl_GetActiveAttrib( GLuint program, GLuint index,
                                   GLsizei  bufSize,
                                   GLsizei* length, GLint* size,
                                   GLenum* type,
                                   GLchar* name ) = 0;

  virtual GLint gl_GetAttribLocation( GLuint        program,
                                      GLchar const* name ) = 0;

  virtual GLenum gl_GetError() = 0;

  virtual void gl_GetIntegerv( GLenum pname, GLint* data ) = 0;

  virtual void gl_GetProgramInfoLog( GLuint   program,
                                     GLsizei  bufSize,
                                     GLsizei* length,
                                     GLchar*  infoLog ) = 0;

  virtual void gl_GetProgramiv( GLuint program, GLenum pname,
                                GLint* params ) = 0;

  virtual void gl_GetShaderInfoLog( GLuint   shader,
                                    GLsizei  bufSize,
                                    GLsizei* length,
                                    GLchar*  infoLog ) = 0;

  virtual void gl_GetShaderiv( GLuint shader, GLenum pname,
                               GLint* params ) = 0;

  virtual GLint gl_GetUniformLocation( GLuint        program,
                                       GLchar const* name ) = 0;

  virtual void gl_LinkProgram( GLuint program ) = 0;

  virtual void gl_ShaderSource( GLuint shader, GLsizei count,
                                GLchar const* const* string,
                                GLint const* length ) = 0;

  virtual void gl_Uniform1f( GLint location, GLfloat v0 ) = 0;

  virtual void gl_Uniform1i( GLint location, GLint v0 ) = 0;

  virtual void gl_Uniform2f( GLint location, GLfloat v0,
                             GLfloat v1 ) = 0;

  virtual void gl_UseProgram( GLuint program ) = 0;

  virtual void gl_ValidateProgram( GLuint program ) = 0;

  virtual void gl_VertexAttribPointer( GLuint index, GLint size,
                                       GLenum      type,
                                       GLboolean   normalized,
                                       GLsizei     stride,
                                       void const* pointer ) = 0;

  virtual void gl_GenTextures( GLsizei n, GLuint* textures ) = 0;

  virtual void gl_DeleteTextures( GLsizei       n,
                                  GLuint const* textures ) = 0;

  virtual void gl_BindTexture( GLenum target,
                               GLuint texture ) = 0;

  virtual void gl_TexParameteri( GLenum target, GLenum pname,
                                 GLint param ) = 0;

  virtual void gl_TexImage2D( GLenum target, GLint level,
                              GLint   internalformat,
                              GLsizei width, GLsizei height,
                              GLint border, GLenum format,
                              GLenum      type,
                              void const* pixels ) = 0;
};

/****************************************************************
** Public API
*****************************************************************/

// Must be set at the start of every program using this library;
// it defauls to nullptr.
IOpenGL* global_gl_implementation();

void set_global_gl_implementation( IOpenGL* iopengl );

} // namespace gl
