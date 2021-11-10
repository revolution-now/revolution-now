/****************************************************************
**uniform.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-02.
*
* Description: OpenGL uniform representation.
*
*****************************************************************/
#include "uniform.hpp"

// gl
#include "error.hpp"
#include "iface.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace gl {

/****************************************************************
** UniformNonTyped
*****************************************************************/
UniformNonTyped::UniformNonTyped( ObjId       pgrm_id,
                                  string_view name )
  : pgrm_id_{ pgrm_id }, location_{} {
  location_ = GL_CHECK( CALL_GL( gl_GetUniformLocation, pgrm_id_,
                                 string( name ).c_str() ) );
  if( location_ == -1 ) {
    FATAL( "active uniform named '{}' not found in program.",
           name );
  }
}

UniformNonTyped::set_valid_t UniformNonTyped::set(
    base::safe::floating<float> val ) const {
  GL_CHECK( CALL_GL( gl_UseProgram, pgrm_id_ ) );
  glUniform1f( location_, val );
  if( print_errors() ) return "failed to set uniform as float";
  return base::valid;
}

UniformNonTyped::set_valid_t UniformNonTyped::set(
    base::safe::integer<long> val ) const {
  GL_CHECK( CALL_GL( gl_UseProgram, pgrm_id_ ) );
  glUniform1i( location_, val );
  if( print_errors() ) return "failed to set uniform as long";
  return base::valid;
}

UniformNonTyped::set_valid_t UniformNonTyped::set(
    base::safe::boolean val ) const {
  GL_CHECK( CALL_GL( gl_UseProgram, pgrm_id_ ) );
  glUniform1i( location_, bool( val ) ? 1 : 0 );
  if( print_errors() ) return "failed to set uniform as bool";
  return base::valid;
}

UniformNonTyped::set_valid_t UniformNonTyped::set(
    vec2 val ) const {
  GL_CHECK( CALL_GL( gl_UseProgram, pgrm_id_ ) );
  glUniform2f( location_, val.x, val.y );
  if( print_errors() ) return "failed to set uniform as vec2";
  return base::valid;
}

} // namespace gl
