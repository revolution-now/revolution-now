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

struct MockOpenGL : IOpenGL {
  MOCK_GL_METHOD( void, gl_AttachShader,
                  ( ( GLuint, program ), ( GLuint, shader ) ) );
  MOCK_GL_METHOD( void, gl_BindBuffer,
                  ( ( GLenum, target ), ( GLuint, buffer ) ) );
  MOCK_GL_METHOD( void, gl_BindVertexArray,
                  ( ( GLuint, array ) ) );
  MOCK_GL_METHOD( void, gl_BufferData,
                  ( ( GLenum, target ), ( GLsizeiptr, size ),
                    ( void const*, data ), ( GLenum, usage ) ) );
  MOCK_GL_METHOD( void, gl_BufferSubData,
                  ( ( GLenum, target ), ( GLintptr, offset ),
                    ( GLsizeiptr, size ),
                    ( const void*, data ) ) );
  MOCK_GL_METHOD( void, gl_CompileShader,
                  ( ( GLuint, shader ) ) );
  MOCK_GL_METHOD( GLuint, gl_CreateProgram, () );
  MOCK_GL_METHOD( GLuint, gl_CreateShader,
                  ( ( GLenum, type ) ) );
  MOCK_GL_METHOD( void, gl_DeleteBuffers,
                  ( ( GLsizei, n ),
                    ( GLuint const*, buffers ) ) );
  MOCK_GL_METHOD( void, gl_DeleteProgram,
                  ( ( GLuint, program ) ) );
  MOCK_GL_METHOD( void, gl_DeleteShader,
                  ( ( GLuint, shader ) ) );
  MOCK_GL_METHOD( void, gl_DeleteVertexArrays,
                  ( ( GLsizei, n ),
                    ( GLuint const*, arrays ) ) );
  MOCK_GL_METHOD( void, gl_DetachShader,
                  ( ( GLuint, program ), ( GLuint, shader ) ) );
  MOCK_GL_METHOD( void, gl_DrawArrays,
                  ( ( GLenum, mode ), ( GLint, first ),
                    ( GLsizei, count ) ) );
  MOCK_GL_METHOD( void, gl_EnableVertexAttribArray,
                  ( ( GLuint, index ) ) );
  MOCK_GL_METHOD( void, gl_GenBuffers,
                  ( ( GLsizei, n ), ( GLuint*, buffers ) ) );
  MOCK_GL_METHOD( void, gl_GenVertexArrays,
                  ( ( GLsizei, n ), ( GLuint*, arrays ) ) );
  MOCK_GL_METHOD( void, gl_GetActiveAttrib,
                  ( ( GLuint, program ), ( GLuint, index ),
                    ( GLsizei, bufSize ), ( GLsizei*, length ),
                    ( GLint*, size ), ( GLenum*, type ),
                    ( GLchar*, name ) ) );
  MOCK_GL_METHOD( GLint, gl_GetAttribLocation,
                  ( ( GLuint, program ),
                    ( GLchar const*, name ) ) );
  MOCK_GL_METHOD( GLenum, gl_GetError, () );
  MOCK_GL_METHOD( void, gl_GetIntegerv,
                  ( ( GLenum, pname ), ( GLint*, data ) ) );
  MOCK_GL_METHOD( void, gl_GetProgramInfoLog,
                  ( ( GLuint, program ), ( GLsizei, bufSize ),
                    ( GLsizei*, length ),
                    ( GLchar*, infoLog ) ) );
  MOCK_GL_METHOD( void, gl_GetProgramiv,
                  ( ( GLuint, program ), ( GLenum, pname ),
                    ( GLint*, params ) ) );
  MOCK_GL_METHOD( void, gl_GetShaderInfoLog,
                  ( ( GLuint, shader ), ( GLsizei, bufSize ),
                    ( GLsizei*, length ),
                    ( GLchar*, infoLog ) ) );
  MOCK_GL_METHOD( void, gl_GetShaderiv,
                  ( ( GLuint, shader ), ( GLenum, pname ),
                    ( GLint*, params ) ) );
  MOCK_GL_METHOD( GLint, gl_GetUniformLocation,
                  ( ( GLuint, program ),
                    ( GLchar const*, name ) ) );
  MOCK_GL_METHOD( void, gl_LinkProgram,
                  ( ( GLuint, program ) ) );
  MOCK_GL_METHOD( void, gl_ShaderSource,
                  ( ( GLuint, shader ), ( GLsizei, count ),
                    ( GLchar const* const*, conststring ),
                    ( GLint const*, length ) ) );
  MOCK_GL_METHOD( void, gl_Uniform1f,
                  ( ( GLint, location ), ( GLfloat, v0 ) ) );
  MOCK_GL_METHOD( void, gl_Uniform1i,
                  ( ( GLint, location ), ( GLint, v0 ) ) );
  MOCK_GL_METHOD( void, gl_Uniform2f,
                  ( ( GLint, location ), ( GLfloat, v0 ),
                    ( GLfloat, v1 ) ) );
  MOCK_GL_METHOD( void, gl_UseProgram, ( ( GLuint, program ) ) );
  MOCK_GL_METHOD( void, gl_ValidateProgram,
                  ( ( GLuint, program ) ) );
  MOCK_GL_METHOD( void, gl_VertexAttribPointer,
                  ( ( GLuint, index ), ( GLint, size ),
                    ( GLenum, type ), ( GLboolean, normalized ),
                    ( GLsizei, stride ),
                    ( void const*, pointer ) ) );
};

static_assert( !std::is_abstract_v<MockOpenGL> );

} // namespace gl
