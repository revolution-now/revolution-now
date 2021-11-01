/****************************************************************
**attribs.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-28.
*
* Description: C++ types corresponding to GL types.
*
*****************************************************************/
#include "attribs.hpp"

// base
#include "base/error.hpp"

// Glad
#include "glad/glad.h"

using namespace std;

namespace gl {

/****************************************************************
** Attribute Type
*****************************************************************/
int to_GL( e_attrib_type type ) {
  decltype( GL_FLOAT ) res = 0;
  switch( type ) {
    case e_attrib_type::float_: res = GL_FLOAT; break;
  }
  return static_cast<int>( res );
}

string_view to_GL_str( e_attrib_type type ) {
  switch( type ) {
    case e_attrib_type::float_: return "GL_FLOAT"; break;
  }
}

/****************************************************************
** Attribute Compound Type
*****************************************************************/
int to_GL( e_attrib_compound_type type ) {
  decltype( GL_FLOAT ) res = 0;
  switch( type ) {
    case e_attrib_compound_type::vec2:
      res = GL_FLOAT_VEC2;
      break;
    case e_attrib_compound_type::vec3:
      res = GL_FLOAT_VEC3;
      break;
  }
  return static_cast<int>( res );
}

string_view to_GL_str( e_attrib_compound_type type ) {
  switch( type ) {
    case e_attrib_compound_type::vec2:
      return "GL_FLOAT_VEC2";
      break;
    case e_attrib_compound_type::vec3:
      return "GL_FLOAT_VEC3";
      break;
  }
}

e_attrib_compound_type from_GL( int type ) {
  switch( type ) {
    case GL_FLOAT_VEC2: return e_attrib_compound_type::vec2;
    case GL_FLOAT_VEC3: return e_attrib_compound_type::vec3;
    default:
      FATAL( "unrecognized OpenGL compound type {}.", type );
  }
}

/****************************************************************
** vec2
*****************************************************************/
e_attrib_type attrib_traits<vec2>::component_type =
    e_attrib_type::float_;
e_attrib_compound_type attrib_traits<vec2>::compound_type =
    e_attrib_compound_type::vec2;
int attrib_traits<vec2>::count = 2;

/****************************************************************
** vec3
*****************************************************************/
e_attrib_type attrib_traits<vec3>::component_type =
    e_attrib_type::float_;
e_attrib_compound_type attrib_traits<vec3>::compound_type =
    e_attrib_compound_type::vec3;
int attrib_traits<vec3>::count = 3;

} // namespace gl
