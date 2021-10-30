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
** vec2
*****************************************************************/
e_attrib_type attrib_traits<vec2>::type  = e_attrib_type::float_;
int           attrib_traits<vec2>::count = 2;

} // namespace gl
