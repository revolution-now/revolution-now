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
    case e_attrib_type::int_:
      res = GL_INT;
      break;
    case e_attrib_type::uint:
      res = GL_UNSIGNED_INT;
      break;
    case e_attrib_type::float_:
      res = GL_FLOAT;
      break;
  }
  return static_cast<int>( res );
}

string_view to_GL_str( e_attrib_type type ) {
  switch( type ) {
    case e_attrib_type::int_:
      return "GL_INT";
    case e_attrib_type::uint:
      return "GL_UNSIGNED_INT";
    case e_attrib_type::float_:
      return "GL_FLOAT";
  }
}

/****************************************************************
** Attribute Compound Type
*****************************************************************/
int to_GL( e_attrib_compound_type type ) {
  decltype( GL_FLOAT ) res = 0;
  switch( type ) {
    case e_attrib_compound_type::int_:
      res = GL_INT;
      break;
    case e_attrib_compound_type::uint:
      res = GL_UNSIGNED_INT;
      break;
    case e_attrib_compound_type::float_:
      res = GL_FLOAT;
      break;
    case e_attrib_compound_type::vec2:
      res = GL_FLOAT_VEC2;
      break;
    case e_attrib_compound_type::vec3:
      res = GL_FLOAT_VEC3;
      break;
    case e_attrib_compound_type::vec4:
      res = GL_FLOAT_VEC4;
      break;
    case e_attrib_compound_type::ivec4:
      res = GL_INT_VEC4;
      break;
  }
  return static_cast<int>( res );
}

string_view to_GL_str( e_attrib_compound_type type ) {
  switch( type ) {
    case e_attrib_compound_type::int_:
      return "GL_INT";
    case e_attrib_compound_type::uint:
      return "GL_UNSIGNED_INT";
    case e_attrib_compound_type::float_:
      return "GL_FLOAT";
    case e_attrib_compound_type::vec2:
      return "GL_FLOAT_VEC2";
    case e_attrib_compound_type::vec3:
      return "GL_FLOAT_VEC3";
    case e_attrib_compound_type::vec4:
      return "GL_FLOAT_VEC4";
    case e_attrib_compound_type::ivec4:
      return "GL_INT_VEC4";
  }
}

e_attrib_compound_type from_GL( int type ) {
  switch( type ) {
    case GL_INT:
      return e_attrib_compound_type::int_;
    case GL_UNSIGNED_INT:
      return e_attrib_compound_type::uint;
    case GL_FLOAT:
      return e_attrib_compound_type::float_;
    case GL_FLOAT_VEC2:
      return e_attrib_compound_type::vec2;
    case GL_FLOAT_VEC3:
      return e_attrib_compound_type::vec3;
    case GL_FLOAT_VEC4:
      return e_attrib_compound_type::vec4;
    case GL_INT_VEC4:
      return e_attrib_compound_type::ivec4;
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

vec2 vec2::from_size( gfx::size s ) {
  return vec2{
    .x = static_cast<float>( s.w ),
    .y = static_cast<float>( s.h ),
  };
}

vec2 vec2::from_dsize( gfx::dsize ds ) {
  return vec2{
    .x = static_cast<float>( ds.w ),
    .y = static_cast<float>( ds.h ),
  };
}

/****************************************************************
** vec4
*****************************************************************/
vec4 vec4::from_rect( gfx::rect r ) {
  return vec4{
    .x = static_cast<float>( r.origin.x ),
    .y = static_cast<float>( r.origin.y ),
    .z = static_cast<float>( r.size.w ),
    .w = static_cast<float>( r.size.h ),
  };
}

vec4 vec4::with_alpha( float a ) const {
  auto copy = *this;
  copy.w    = a;
  return copy;
}

/****************************************************************
** ivec4
*****************************************************************/
ivec4 ivec4::from_rect( gfx::rect const r ) {
  return ivec4{
    .x = r.origin.x,
    .y = r.origin.y,
    .z = r.size.w,
    .w = r.size.h,
  };
}

ivec4 ivec4::from_pixel( gfx::pixel const p ) {
  return ivec4{
    .x = p.r,
    .y = p.g,
    .z = p.b,
    .w = p.a,
  };
}

ivec4 ivec4::with_alpha( int32_t a ) const {
  auto copy = *this;
  copy.w    = a;
  return copy;
}

/****************************************************************
** color
*****************************************************************/
color color::from_pixel( gfx::pixel p ) {
  return color{
    .r = static_cast<float>( p.r ) / 255.0f,
    .g = static_cast<float>( p.g ) / 255.0f,
    .b = static_cast<float>( p.b ) / 255.0f,
    .a = static_cast<float>( p.a ) / 255.0f,
  };
}

} // namespace gl
