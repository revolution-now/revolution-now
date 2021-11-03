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

// base
#include "base/error.hpp"

// Glad
#include "glad/glad.h"

using namespace std;

namespace gl {

/****************************************************************
** UniformNonTyped
*****************************************************************/
UniformNonTyped::UniformNonTyped( ObjId       pgrm_id,
                                  string_view name )
  : pgrm_id_{ pgrm_id }, location_{} {
  location_ = GL_CHECK(
      glGetUniformLocation( pgrm_id_, string( name ).c_str() ) );
  if( location_ == -1 ) {
    FATAL( "active uniform named '{}' not found in program.",
           name );
  }
}

void UniformNonTyped::set(
    base::safe::floating<float> val ) const {
  GL_CHECK( glUseProgram( pgrm_id_ ) );
  GL_CHECK( glUniform1f( location_, val ) );
}

void UniformNonTyped::set(
    base::safe::integer<long> val ) const {
  GL_CHECK( glUseProgram( pgrm_id_ ) );
  GL_CHECK( glUniform1i( location_, val ) );
}

void UniformNonTyped::set( base::safe::boolean val ) const {
  GL_CHECK( glUseProgram( pgrm_id_ ) );
  GL_CHECK( glUniform1i( location_, bool( val ) ? 1 : 0 ) );
}

void UniformNonTyped::set( vec2 val ) const {
  GL_CHECK( glUseProgram( pgrm_id_ ) );
  GL_CHECK( glUniform2f( location_, val.x, val.y ) );
}

} // namespace gl
