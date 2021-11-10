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

// C++ standard library
#include <type_traits>

#define MOCK_GL_METHOD( name, ret_type, params ) \
  ret_type name params override

namespace gl {

struct MockOpenGL : IOpenGL {
  MOCK_GL_METHOD( gl_AttachShader, void,
                  ( GLuint program, GLuint shader ) );
  MOCK_GL_METHOD( gl_BindBuffer, void,
                  ( GLenum target, GLuint buffer ) );
  MOCK_GL_METHOD( gl_BindVertexArray, void, ( GLuint array ) );
  MOCK_GL_METHOD( gl_BufferData, void,
                  ( GLenum target, GLsizeiptr size,
                    void const* data, GLenum usage ) );
  MOCK_GL_METHOD( gl_BufferSubData, void,
                  ( GLenum target, GLintptr offset,
                    GLsizeiptr size, const void* data ) );
  MOCK_GL_METHOD( gl_CompileShader, void, ( GLuint shader ) );
  MOCK_GL_METHOD( gl_CreateProgram, GLuint, (void));
  MOCK_GL_METHOD( gl_CreateShader, GLuint, ( GLenum type ) );
  MOCK_GL_METHOD( gl_DeleteBuffers, void,
                  ( GLsizei n, GLuint const* buffers ) );
  MOCK_GL_METHOD( gl_DeleteProgram, void, ( GLuint program ) );
  MOCK_GL_METHOD( gl_DeleteShader, void, ( GLuint shader ) );
  MOCK_GL_METHOD( gl_DeleteVertexArrays, void,
                  ( GLsizei n, GLuint const* arrays ) );
  MOCK_GL_METHOD( gl_DetachShader, void,
                  ( GLuint program, GLuint shader ) );
  MOCK_GL_METHOD( gl_DrawArrays, void,
                  ( GLenum mode, GLint first, GLsizei count ) );
  MOCK_GL_METHOD( gl_EnableVertexAttribArray, void,
                  ( GLuint index ) );
  MOCK_GL_METHOD( gl_GenBuffers, void,
                  ( GLsizei n, GLuint* buffers ) );
  MOCK_GL_METHOD( gl_GenVertexArrays, void,
                  ( GLsizei n, GLuint* arrays ) );
  MOCK_GL_METHOD( gl_GetActiveAttrib, void,
                  ( GLuint program, GLuint index,
                    GLsizei bufSize, GLsizei* length,
                    GLint* size, GLenum* type, GLchar* name ) );
  MOCK_GL_METHOD( gl_GetAttribLocation, GLint,
                  ( GLuint program, GLchar const* name ) );
  MOCK_GL_METHOD( gl_GetError, GLenum, (void));
  MOCK_GL_METHOD( gl_GetIntegerv, void,
                  ( GLenum pname, GLint* data ) );
  MOCK_GL_METHOD( gl_GetProgramInfoLog, void,
                  ( GLuint program, GLsizei bufSize,
                    GLsizei* length, GLchar* infoLog ) );
  MOCK_GL_METHOD( gl_GetProgramiv, void,
                  ( GLuint program, GLenum pname,
                    GLint* params ) );
  MOCK_GL_METHOD( gl_GetShaderInfoLog, void,
                  ( GLuint shader, GLsizei bufSize,
                    GLsizei* length, GLchar* infoLog ) );
  MOCK_GL_METHOD( gl_GetShaderiv, void,
                  ( GLuint shader, GLenum pname,
                    GLint* params ) );
  MOCK_GL_METHOD( gl_GetUniformLocation, GLint,
                  ( GLuint program, GLchar const* name ) );
  MOCK_GL_METHOD( gl_LinkProgram, void, ( GLuint program ) );
  MOCK_GL_METHOD( gl_ShaderSource, void,
                  ( GLuint shader, GLsizei count,
                    GLchar const* const* conststring,
                    GLint const*         length ) );
  MOCK_GL_METHOD( gl_Uniform1f, void,
                  ( GLint location, GLfloat v0 ) );
  MOCK_GL_METHOD( gl_Uniform1i, void,
                  ( GLint location, GLint v0 ) );
  MOCK_GL_METHOD( gl_Uniform2f, void,
                  ( GLint location, GLfloat v0, GLfloat v1 ) );
  MOCK_GL_METHOD( gl_UseProgram, void, ( GLuint program ) );
  MOCK_GL_METHOD( gl_ValidateProgram, void, ( GLuint program ) );
  MOCK_GL_METHOD( gl_VertexAttribPointer, void,
                  ( GLuint index, GLint size, GLenum type,
                    GLboolean normalized, GLsizei stride,
                    void const* pointer ) );
};

static_assert( !std::is_abstract_v<MockOpenGL> );

} // namespace gl
