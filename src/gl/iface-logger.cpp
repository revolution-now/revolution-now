/****************************************************************
**iface-logger.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Implementation of IOpenGL that logs and forwards.
*
*****************************************************************/
#include "iface-logger.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/keyval.hpp"
#include "base/macros.hpp"

// base-util
#include "base-util/pp.hpp"

// C++ standard library
#include <unordered_set>

using namespace std;

#define ADD_TO_PARAM_STR( var )                     \
  param_str =                                       \
      fmt::format( "/*{}=*/{}, ", TO_STRING( var ), \
                   fmt::to_string( cast_to_void( var ) ) )

#define EXPAND_PARAM( type, name ) type name

// `params` is a list of pairs.
#define LOG_AND_CALL_GL_METHOD( name, ret_type, params )      \
  EVAL( ret_type OpenGLWithLogger::name( PP_MAP_TUPLE_COMMAS( \
      EXPAND_PARAM, PP_REMOVE_PARENS params ) ) {             \
    if( logging_enabled_ ) {                                  \
      string param_str;                                       \
      PP_MAP_SEMI(                                            \
          ADD_TO_PARAM_STR,                                   \
          PP_MAP_TUPLE_COMMAS( PP_PAIR_TAKE_SECOND,           \
                               PP_REMOVE_PARENS params ) );   \
      if( !param_str.empty() )                                \
        param_str.resize( param_str.size() - 2 );             \
      log_gl_call( #name, param_str );                        \
    }                                                         \
    return next_->name( PP_MAP_TUPLE_COMMAS(                  \
        PP_PAIR_TAKE_SECOND, PP_REMOVE_PARENS params ) );     \
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

unordered_set<string_view> no_log{
    "gl_GetError",
};

void log_gl_call( string_view name, string_view params ) {
  DCHECK( name.starts_with( "gl_" ) );
  if( base::find( no_log, name ) ) return;
  string no_prefix( name.begin() + 3, name.end() );
  fmt::print( "OpenGLWithLogger: gl{}( {} )\n", no_prefix,
              params );
}

} // namespace

void OpenGLWithLogger::enable_logging( bool enabled ) {
  logging_enabled_ = enabled;
}

LOG_AND_CALL_GL_METHOD( gl_AttachShader, void,
                        ( ( GLuint, program ),
                          ( GLuint, shader ) ) );

LOG_AND_CALL_GL_METHOD( gl_BindBuffer, void,
                        ( ( GLenum, target ),
                          ( GLuint, buffer ) ) );

LOG_AND_CALL_GL_METHOD( gl_BindVertexArray, void,
                        ( ( GLuint, array ) ) );

LOG_AND_CALL_GL_METHOD( gl_BufferData, void,
                        ( ( GLenum, target ),
                          ( GLsizeiptr, size ),
                          ( const void*, data ),
                          ( GLenum, usage ) ) );

LOG_AND_CALL_GL_METHOD( gl_BufferSubData, void,
                        ( ( GLenum, target ),
                          ( GLintptr, offset ),
                          ( GLsizeiptr, size ),
                          ( const void*, data ) ) );

LOG_AND_CALL_GL_METHOD( gl_CompileShader, void,
                        ( ( GLuint, shader ) ) );

LOG_AND_CALL_GL_METHOD( gl_CreateProgram, GLuint, () );

LOG_AND_CALL_GL_METHOD( gl_CreateShader, GLuint,
                        ( ( GLenum, type ) ) );

LOG_AND_CALL_GL_METHOD( gl_DeleteBuffers, void,
                        ( ( GLsizei, n ),
                          ( const GLuint*, buffers ) ) );

LOG_AND_CALL_GL_METHOD( gl_DeleteProgram, void,
                        ( ( GLuint, program ) ) );

LOG_AND_CALL_GL_METHOD( gl_DeleteShader, void,
                        ( ( GLuint, shader ) ) );

LOG_AND_CALL_GL_METHOD( gl_DeleteVertexArrays, void,
                        ( ( GLsizei, n ),
                          ( const GLuint*, arrays ) ) );

LOG_AND_CALL_GL_METHOD( gl_DetachShader, void,
                        ( ( GLuint, program ),
                          ( GLuint, shader ) ) );

LOG_AND_CALL_GL_METHOD( gl_DrawArrays, void,
                        ( ( GLenum, mode ), ( GLint, first ),
                          ( GLsizei, count ) ) );

LOG_AND_CALL_GL_METHOD( gl_EnableVertexAttribArray, void,
                        ( ( GLuint, index ) ) );

LOG_AND_CALL_GL_METHOD( gl_GenBuffers, void,
                        ( ( GLsizei, n ),
                          ( GLuint*, buffers ) ) );

LOG_AND_CALL_GL_METHOD( gl_GenVertexArrays, void,
                        ( ( GLsizei, n ),
                          ( GLuint*, arrays ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetActiveAttrib, void,
                        ( ( GLuint, program ), ( GLuint, index ),
                          ( GLsizei, bufSize ),
                          ( GLsizei*, length ), ( GLint*, size ),
                          ( GLenum*, type ),
                          ( GLchar*, name ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetAttribLocation, GLint,
                        ( ( GLuint, program ),
                          ( const GLchar*, name ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetError, GLenum, () );

LOG_AND_CALL_GL_METHOD( gl_GetIntegerv, void,
                        ( ( GLenum, pname ),
                          ( GLint*, data ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetProgramInfoLog, void,
                        ( ( GLuint, program ),
                          ( GLsizei, bufSize ),
                          ( GLsizei*, length ),
                          ( GLchar*, infoLog ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetProgramiv, void,
                        ( ( GLuint, program ), ( GLenum, pname ),
                          ( GLint*, params ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetShaderInfoLog, void,
                        ( ( GLuint, shader ),
                          ( GLsizei, bufSize ),
                          ( GLsizei*, length ),
                          ( GLchar*, infoLog ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetShaderiv, void,
                        ( ( GLuint, shader ), ( GLenum, pname ),
                          ( GLint*, params ) ) );

LOG_AND_CALL_GL_METHOD( gl_GetUniformLocation, GLint,
                        ( ( GLuint, program ),
                          ( const GLchar*, name ) ) );

LOG_AND_CALL_GL_METHOD( gl_LinkProgram, void,
                        ( ( GLuint, program ) ) );

LOG_AND_CALL_GL_METHOD( gl_ShaderSource, void,
                        ( ( GLuint, shader ), ( GLsizei, count ),
                          ( const GLchar* const*, str ),
                          ( const GLint*, length ) ) );

LOG_AND_CALL_GL_METHOD( gl_Uniform1f, void,
                        ( ( GLint, location ),
                          ( GLfloat, v0 ) ) );

LOG_AND_CALL_GL_METHOD( gl_Uniform1i, void,
                        ( ( GLint, location ), ( GLint, v0 ) ) );

LOG_AND_CALL_GL_METHOD( gl_Uniform2f, void,
                        ( ( GLint, location ), ( GLfloat, v0 ),
                          ( GLfloat, v1 ) ) );

LOG_AND_CALL_GL_METHOD( gl_UseProgram, void,
                        ( ( GLuint, program ) ) );

LOG_AND_CALL_GL_METHOD( gl_ValidateProgram, void,
                        ( ( GLuint, program ) ) );

LOG_AND_CALL_GL_METHOD( gl_VertexAttribPointer, void,
                        ( ( GLuint, index ), ( GLint, size ),
                          ( GLenum, type ),
                          ( GLboolean, normalized ),
                          ( GLsizei, stride ),
                          ( const void*, pointer ) ) );

} // namespace gl
