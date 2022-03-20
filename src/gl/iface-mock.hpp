/****************************************************************
**iface-mock.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Mock implementation of IOpenGL.
*
*****************************************************************/
#pragma once

// gl
#include "iface.hpp"

// mock
#include "mock/mock.hpp"

// C++ standard library
#include <type_traits>

#define MOCK_GL_METHOD( ret_type, name, params ) \
  MOCK_METHOD( ret_type, name, params, () )

namespace gl {

// This is a special implementation of IOpenGL that not only pro-
// vides mocked methods, but also sets/restores the global
// IOpenGL instance uponn construction/destruction.
struct MockOpenGL : IOpenGL {
 private:
  IOpenGL* prev_;

 public:
  MockOpenGL();

  ~MockOpenGL();

  MOCK_GL_METHOD( void, gl_AttachShader, ( GLuint, GLuint ) );
  MOCK_GL_METHOD( void, gl_BindBuffer, ( GLenum, GLuint ) );
  MOCK_GL_METHOD( void, gl_BindVertexArray, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_BufferData,
                  ( GLenum, GLsizeiptr, void const*, GLenum ) );
  MOCK_GL_METHOD( void, gl_BufferSubData,
                  (GLenum, GLintptr, GLsizeiptr, const void*));
  MOCK_GL_METHOD( void, gl_CompileShader, ( GLuint ) );
  MOCK_GL_METHOD( GLuint, gl_CreateProgram, () );
  MOCK_GL_METHOD( GLuint, gl_CreateShader, ( GLenum ) );
  MOCK_GL_METHOD( void, gl_DeleteBuffers,
                  (GLsizei, GLuint const*));
  MOCK_GL_METHOD( void, gl_DeleteProgram, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_DeleteShader, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_DeleteVertexArrays,
                  (GLsizei, GLuint const*));
  MOCK_GL_METHOD( void, gl_DetachShader, ( GLuint, GLuint ) );
  MOCK_GL_METHOD( void, gl_DrawArrays,
                  ( GLenum, GLint, GLsizei ) );
  MOCK_GL_METHOD( void, gl_EnableVertexAttribArray, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_GenBuffers, (GLsizei, GLuint*));
  MOCK_GL_METHOD( void, gl_GenVertexArrays, (GLsizei, GLuint*));
  MOCK_GL_METHOD( void, gl_GetActiveAttrib,
                  (GLuint, GLuint, GLsizei, GLsizei*, GLint*,
                   GLenum*, GLchar*));
  MOCK_GL_METHOD( GLint, gl_GetAttribLocation,
                  (GLuint, GLchar const*));
  MOCK_GL_METHOD( GLenum, gl_GetError, () );
  MOCK_GL_METHOD( void, gl_GetIntegerv, (GLenum, GLint*));
  MOCK_GL_METHOD( void, gl_GetProgramInfoLog,
                  (GLuint, GLsizei, GLsizei*, GLchar*));
  MOCK_GL_METHOD( void, gl_GetProgramiv,
                  (GLuint, GLenum, GLint*));
  MOCK_GL_METHOD( void, gl_GetShaderInfoLog,
                  (GLuint, GLsizei, GLsizei*, GLchar*));
  MOCK_GL_METHOD( void, gl_GetShaderiv,
                  (GLuint, GLenum, GLint*));
  MOCK_GL_METHOD( GLint, gl_GetUniformLocation,
                  (GLuint, GLchar const*));
  MOCK_GL_METHOD( void, gl_LinkProgram, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_ShaderSource,
                  (GLuint, GLsizei, GLchar const* const*,
                   GLint const*));
  MOCK_GL_METHOD( void, gl_Uniform1f, ( GLint, GLfloat ) );
  MOCK_GL_METHOD( void, gl_Uniform1i, ( GLint, GLint ) );
  MOCK_GL_METHOD( void, gl_Uniform2f,
                  ( GLint, GLfloat, GLfloat ) );
  MOCK_GL_METHOD( void, gl_UseProgram, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_ValidateProgram, ( GLuint ) );
  MOCK_GL_METHOD( void, gl_VertexAttribPointer,
                  (GLuint, GLint, GLenum, GLboolean, GLsizei,
                   void const*));
  MOCK_GL_METHOD( void, gl_VertexAttribIPointer,
                  (GLuint, GLint, GLenum, GLsizei, void const*));
  MOCK_GL_METHOD( void, gl_GenTextures, (GLsizei, GLuint*));

  MOCK_GL_METHOD( void, gl_DeleteTextures,
                  (GLsizei, GLuint const*));

  MOCK_GL_METHOD( void, gl_BindTexture, ( GLenum, GLuint ) );

  MOCK_GL_METHOD( void, gl_TexParameteri,
                  ( GLenum, GLenum, GLint ) );

  MOCK_GL_METHOD( void, gl_TexImage2D,
                  (GLenum, GLint, GLint, GLsizei, GLsizei, GLint,
                   GLenum, GLenum, void const*));

  MOCK_GL_METHOD( void, gl_Viewport,
                  ( GLint, GLint, GLsizei, GLsizei ) );
};

static_assert( !std::is_abstract_v<MockOpenGL> );

} // namespace gl
