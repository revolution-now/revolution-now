/****************************************************************
**attribs.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-28.
*
* Description: C++ types corresponding to GL types.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <string_view>

namespace gl {

/****************************************************************
** Attribute Type
*****************************************************************/
enum class e_attrib_type { float_ };

int to_GL( e_attrib_type type );

std::string_view to_GL_str( e_attrib_type type );

/****************************************************************
** Attribute Compound Type
*****************************************************************/
enum class e_attrib_compound_type { vec2, vec3 };

int to_GL( e_attrib_compound_type type );

e_attrib_compound_type from_GL( int type );

std::string_view to_GL_str( e_attrib_compound_type type );

/****************************************************************
** Attribute Type Traits
*****************************************************************/
template<typename T>
struct attrib_traits;

/****************************************************************
** vec2
*****************************************************************/
struct vec2 {
  float x;
  float y;
};

template<>
struct attrib_traits<vec2> {
  static e_attrib_type          component_type;
  static e_attrib_compound_type compound_type;
  static int                    count;
};

/****************************************************************
** vec3
*****************************************************************/
struct vec3 {
  float x;
  float y;
  float z;
};

template<>
struct attrib_traits<vec3> {
  static e_attrib_type          component_type;
  static e_attrib_compound_type compound_type;
  static int                    count;
};

} // namespace gl
