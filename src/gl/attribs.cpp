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

// gl
#include "iface.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace gl {

/****************************************************************
** Attribute Type
*****************************************************************/
int to_GL( e_attrib_type type ) {
  decltype( GL_FLOAT ) res = 0;
  switch( type ) {
    case e_attrib_type::int_: res = GL_INT; break;
    case e_attrib_type::float_: res = GL_FLOAT; break;
  }
  return static_cast<int>( res );
}

string_view to_GL_str( e_attrib_type type ) {
  switch( type ) {
    case e_attrib_type::int_: return "GL_INT"; break;
    case e_attrib_type::float_: return "GL_FLOAT"; break;
  }
}

/****************************************************************
** Attribute Compound Type
*****************************************************************/
int to_GL( e_attrib_compound_type type ) {
  decltype( GL_FLOAT ) res = 0;
  switch( type ) {
    case e_attrib_compound_type::int_: res = GL_INT; break;
    case e_attrib_compound_type::float_: res = GL_FLOAT; break;
    case e_attrib_compound_type::vec2:
      res = GL_FLOAT_VEC2;
      break;
    case e_attrib_compound_type::vec3:
      res = GL_FLOAT_VEC3;
      break;
    case e_attrib_compound_type::vec4:
      res = GL_FLOAT_VEC4;
      break;
  }
  return static_cast<int>( res );
}

string_view to_GL_str( e_attrib_compound_type type ) {
  switch( type ) {
    case e_attrib_compound_type::int_: return "GL_INT"; break;
    case e_attrib_compound_type::float_:
      return "GL_FLOAT";
      break;
    case e_attrib_compound_type::vec2:
      return "GL_FLOAT_VEC2";
      break;
    case e_attrib_compound_type::vec3:
      return "GL_FLOAT_VEC3";
      break;
    case e_attrib_compound_type::vec4:
      return "GL_FLOAT_VEC4";
      break;
  }
}

e_attrib_compound_type from_GL( int type ) {
  switch( type ) {
    case GL_INT: return e_attrib_compound_type::int_;
    case GL_FLOAT: return e_attrib_compound_type::float_;
    case GL_FLOAT_VEC2: return e_attrib_compound_type::vec2;
    case GL_FLOAT_VEC3: return e_attrib_compound_type::vec3;
    case GL_FLOAT_VEC4: return e_attrib_compound_type::vec4;
    default:
      FATAL( "unrecognized OpenGL compound type {}.", type );
  }
}

/****************************************************************
** vec2
*****************************************************************/
vec2 vec2::from_point( gfx::point p ) {
  return vec2{
      .x = static_cast<float>( p.x ),
      .y = static_cast<float>( p.y ),
  };
}

/****************************************************************
** color
*****************************************************************/
color color::from_pixel( gfx::pixel p ) {
  return color{
      .r = static_cast<float>( p.r ) / 256.0f,
      .g = static_cast<float>( p.g ) / 256.0f,
      .b = static_cast<float>( p.b ) / 256.0f,
      .a = static_cast<float>( p.a ) / 256.0f,
  };
}

} // namespace gl
