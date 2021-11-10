/****************************************************************
**iface-mock.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Mock implementation of IOpenGL.
*
*****************************************************************/
#include "iface-mock.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"

// base-util
#include "base-util/pp.hpp"

using namespace std;

#define ADD_TO_PARAM_STR( var )                     \
  param_str =                                       \
      fmt::format( "/*{}=*/{}, ", TO_STRING( var ), \
                   fmt::to_string( cast_to_void( var ) ) )

#define EXPAND_PARAM( type, name ) type name

// `params` is a list of pairs.
#define MOCK_GL_METHOD_IMPL( name, ret_type, params )     \
  EVAL( ret_type MockOpenGL::name( PP_MAP_TUPLE_COMMAS(   \
      EXPAND_PARAM, PP_REMOVE_PARENS params ) ) {         \
    string param_str;                                     \
    PP_MAP_SEMI(                                          \
        ADD_TO_PARAM_STR,                                 \
        PP_MAP_TUPLE_COMMAS( PP_PAIR_TAKE_SECOND,         \
                             PP_REMOVE_PARENS params ) ); \
    if( !param_str.empty() )                              \
      param_str.resize( param_str.size() - 2 );           \
    FATAL( "unhandled mock call: {}( {} ).", #name,       \
           param_str );                                   \
  } )

namespace gl {

namespace {

template<typename T>
auto cast_to_void( T&& arg ) {
  if constexpr( is_pointer_v<remove_cvref_t<T>> )
    return (void*)arg;
  else
    return arg;
}

} // namespace

MOCK_GL_METHOD_IMPL( gl_AttachShader, void,
                     ( ( GLuint, program ),
                       ( GLuint, shader ) ) );

MOCK_GL_METHOD_IMPL( gl_BindBuffer, void,
                     ( ( GLenum, target ),
                       ( GLuint, buffer ) ) );

MOCK_GL_METHOD_IMPL( gl_BindVertexArray, void,
                     ( ( GLuint, array ) ) );

MOCK_GL_METHOD_IMPL( gl_BufferData, void,
                     ( ( GLenum, target ), ( GLsizeiptr, size ),
                       ( const void*, data ),
                       ( GLenum, usage ) ) );

MOCK_GL_METHOD_IMPL( gl_BufferSubData, void,
                     ( ( GLenum, target ), ( GLintptr, offset ),
                       ( GLsizeiptr, size ),
                       ( const void*, data ) ) );

MOCK_GL_METHOD_IMPL( gl_CompileShader, void,
                     ( ( GLuint, shader ) ) );

MOCK_GL_METHOD_IMPL( gl_CreateProgram, GLuint, () );

MOCK_GL_METHOD_IMPL( gl_CreateShader, GLuint,
                     ( ( GLenum, type ) ) );

MOCK_GL_METHOD_IMPL( gl_DeleteBuffers, void,
                     ( ( GLsizei, n ),
                       ( const GLuint*, buffers ) ) );

MOCK_GL_METHOD_IMPL( gl_DeleteProgram, void,
                     ( ( GLuint, program ) ) );

MOCK_GL_METHOD_IMPL( gl_DeleteShader, void,
                     ( ( GLuint, shader ) ) );

MOCK_GL_METHOD_IMPL( gl_DeleteVertexArrays, void,
                     ( ( GLsizei, n ),
                       ( const GLuint*, arrays ) ) );

MOCK_GL_METHOD_IMPL( gl_DetachShader, void,
                     ( ( GLuint, program ),
                       ( GLuint, shader ) ) );

MOCK_GL_METHOD_IMPL( gl_DrawArrays, void,
                     ( ( GLenum, mode ), ( GLint, first ),
                       ( GLsizei, count ) ) );

MOCK_GL_METHOD_IMPL( gl_EnableVertexAttribArray, void,
                     ( ( GLuint, index ) ) );

MOCK_GL_METHOD_IMPL( gl_GenBuffers, void,
                     ( ( GLsizei, n ), ( GLuint*, buffers ) ) );

MOCK_GL_METHOD_IMPL( gl_GenVertexArrays, void,
                     ( ( GLsizei, n ), ( GLuint*, arrays ) ) );

MOCK_GL_METHOD_IMPL( gl_GetActiveAttrib, void,
                     ( ( GLuint, program ), ( GLuint, index ),
                       ( GLsizei, bufSize ),
                       ( GLsizei*, length ), ( GLint*, size ),
                       ( GLenum*, type ), ( GLchar*, name ) ) );

MOCK_GL_METHOD_IMPL( gl_GetAttribLocation, GLint,
                     ( ( GLuint, program ),
                       ( const GLchar*, name ) ) );

MOCK_GL_METHOD_IMPL( gl_GetError, GLenum, () );

MOCK_GL_METHOD_IMPL( gl_GetIntegerv, void,
                     ( ( GLenum, pname ), ( GLint*, data ) ) );

MOCK_GL_METHOD_IMPL( gl_GetProgramInfoLog, void,
                     ( ( GLuint, program ), ( GLsizei, bufSize ),
                       ( GLsizei*, length ),
                       ( GLchar*, infoLog ) ) );

MOCK_GL_METHOD_IMPL( gl_GetProgramiv, void,
                     ( ( GLuint, program ), ( GLenum, pname ),
                       ( GLint*, params ) ) );

MOCK_GL_METHOD_IMPL( gl_GetShaderInfoLog, void,
                     ( ( GLuint, shader ), ( GLsizei, bufSize ),
                       ( GLsizei*, length ),
                       ( GLchar*, infoLog ) ) );

MOCK_GL_METHOD_IMPL( gl_GetShaderiv, void,
                     ( ( GLuint, shader ), ( GLenum, pname ),
                       ( GLint*, params ) ) );

MOCK_GL_METHOD_IMPL( gl_GetUniformLocation, GLint,
                     ( ( GLuint, program ),
                       ( const GLchar*, name ) ) );

MOCK_GL_METHOD_IMPL( gl_LinkProgram, void,
                     ( ( GLuint, program ) ) );

MOCK_GL_METHOD_IMPL( gl_ShaderSource, void,
                     ( ( GLuint, shader ), ( GLsizei, count ),
                       ( const GLchar* const*, str ),
                       ( const GLint*, length ) ) );

MOCK_GL_METHOD_IMPL( gl_Uniform1f, void,
                     ( ( GLint, location ), ( GLfloat, v0 ) ) );

MOCK_GL_METHOD_IMPL( gl_Uniform1i, void,
                     ( ( GLint, location ), ( GLint, v0 ) ) );

MOCK_GL_METHOD_IMPL( gl_Uniform2f, void,
                     ( ( GLint, location ), ( GLfloat, v0 ),
                       ( GLfloat, v1 ) ) );

MOCK_GL_METHOD_IMPL( gl_UseProgram, void,
                     ( ( GLuint, program ) ) );

MOCK_GL_METHOD_IMPL( gl_ValidateProgram, void,
                     ( ( GLuint, program ) ) );

MOCK_GL_METHOD_IMPL( gl_VertexAttribPointer, void,
                     ( ( GLuint, index ), ( GLint, size ),
                       ( GLenum, type ),
                       ( GLboolean, normalized ),
                       ( GLsizei, stride ),
                       ( const void*, pointer ) ) );

} // namespace gl
