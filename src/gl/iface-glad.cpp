/****************************************************************
**iface-glad.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Implementation of IOpenGL that calls Glad.
*
*****************************************************************/
#include "iface-glad.hpp"

// base-util
#include "base-util/pp.hpp"

using namespace std;

#define EXPAND_PARAM( type, name ) type name

// `params` is a list of pairs.
#define GLAD_GL_METHOD( name, ret_type, params )             \
  EVAL( ret_type OpenGLGlad::gl_##name( PP_MAP_TUPLE_COMMAS( \
      EXPAND_PARAM, PP_REMOVE_PARENS params ) ) {            \
    return glad_gl##name( PP_MAP_TUPLE_COMMAS(               \
        PP_PAIR_TAKE_SECOND, PP_REMOVE_PARENS params ) );    \
  } )

namespace gl {

GLAD_GL_METHOD( AttachShader, void,
                ( ( GLuint, program ), ( GLuint, shader ) ) );

GLAD_GL_METHOD( BindBuffer, void,
                ( ( GLenum, target ), ( GLuint, buffer ) ) );

GLAD_GL_METHOD( BindVertexArray, void, ( ( GLuint, array ) ) );

GLAD_GL_METHOD( BufferData, void,
                ( ( GLenum, target ), ( GLsizeiptr, size ),
                  ( const void*, data ), ( GLenum, usage ) ) );

GLAD_GL_METHOD( BufferSubData, void,
                ( ( GLenum, target ), ( GLintptr, offset ),
                  ( GLsizeiptr, size ),
                  ( const void*, data ) ) );

GLAD_GL_METHOD( CompileShader, void, ( ( GLuint, shader ) ) );

GLAD_GL_METHOD( CreateProgram, GLuint, () );

GLAD_GL_METHOD( CreateShader, GLuint, ( ( GLenum, type ) ) );

GLAD_GL_METHOD( DeleteBuffers, void,
                ( ( GLsizei, n ), ( const GLuint*, buffers ) ) );

GLAD_GL_METHOD( DeleteProgram, void, ( ( GLuint, program ) ) );

GLAD_GL_METHOD( DeleteShader, void, ( ( GLuint, shader ) ) );

GLAD_GL_METHOD( DeleteVertexArrays, void,
                ( ( GLsizei, n ), ( const GLuint*, arrays ) ) );

GLAD_GL_METHOD( DetachShader, void,
                ( ( GLuint, program ), ( GLuint, shader ) ) );

GLAD_GL_METHOD( DrawArrays, void,
                ( ( GLenum, mode ), ( GLint, first ),
                  ( GLsizei, count ) ) );

GLAD_GL_METHOD( EnableVertexAttribArray, void,
                ( ( GLuint, index ) ) );

GLAD_GL_METHOD( GenBuffers, void,
                ( ( GLsizei, n ), ( GLuint*, buffers ) ) );

GLAD_GL_METHOD( GenVertexArrays, void,
                ( ( GLsizei, n ), ( GLuint*, arrays ) ) );

GLAD_GL_METHOD( GetActiveAttrib, void,
                ( ( GLuint, program ), ( GLuint, index ),
                  ( GLsizei, bufSize ), ( GLsizei*, length ),
                  ( GLint*, size ), ( GLenum*, type ),
                  ( GLchar*, name ) ) );

GLAD_GL_METHOD( GetAttribLocation, GLint,
                ( ( GLuint, program ),
                  ( const GLchar*, name ) ) );

GLAD_GL_METHOD( GetError, GLenum, () );

GLAD_GL_METHOD( GetIntegerv, void,
                ( ( GLenum, pname ), ( GLint*, data ) ) );

GLAD_GL_METHOD( GetProgramInfoLog, void,
                ( ( GLuint, program ), ( GLsizei, bufSize ),
                  ( GLsizei*, length ), ( GLchar*, infoLog ) ) );

GLAD_GL_METHOD( GetProgramiv, void,
                ( ( GLuint, program ), ( GLenum, pname ),
                  ( GLint*, params ) ) );

GLAD_GL_METHOD( GetShaderInfoLog, void,
                ( ( GLuint, shader ), ( GLsizei, bufSize ),
                  ( GLsizei*, length ), ( GLchar*, infoLog ) ) );

GLAD_GL_METHOD( GetShaderiv, void,
                ( ( GLuint, shader ), ( GLenum, pname ),
                  ( GLint*, params ) ) );

GLAD_GL_METHOD( GetUniformLocation, GLint,
                ( ( GLuint, program ),
                  ( const GLchar*, name ) ) );

GLAD_GL_METHOD( LinkProgram, void, ( ( GLuint, program ) ) );

GLAD_GL_METHOD( ShaderSource, void,
                ( ( GLuint, shader ), ( GLsizei, count ),
                  ( const GLchar* const*, str ),
                  ( const GLint*, length ) ) );

GLAD_GL_METHOD( Uniform1f, void,
                ( ( GLint, location ), ( GLfloat, v0 ) ) );

GLAD_GL_METHOD( Uniform1i, void,
                ( ( GLint, location ), ( GLint, v0 ) ) );

GLAD_GL_METHOD( Uniform2f, void,
                ( ( GLint, location ), ( GLfloat, v0 ),
                  ( GLfloat, v1 ) ) );

GLAD_GL_METHOD( UseProgram, void, ( ( GLuint, program ) ) );

GLAD_GL_METHOD( ValidateProgram, void, ( ( GLuint, program ) ) );

GLAD_GL_METHOD( VertexAttribPointer, void,
                ( ( GLuint, index ), ( GLint, size ),
                  ( GLenum, type ), ( GLboolean, normalized ),
                  ( GLsizei, stride ),
                  ( const void*, pointer ) ) );

} // namespace gl
