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
enum class e_attrib_compound_type { float_, vec2, vec3 };

int to_GL( e_attrib_compound_type type );

e_attrib_compound_type from_GL( int type );

std::string_view to_GL_str( e_attrib_compound_type type );

/****************************************************************
** Attribute Type Traits
*****************************************************************/
template<typename T>
struct attrib_traits;

/****************************************************************
** float
*****************************************************************/
template<>
struct attrib_traits<float> {
  inline static e_attrib_type component_type =
      e_attrib_type::float_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::float_;
  inline static int count = 1;
};

/****************************************************************
** vec2
*****************************************************************/
struct vec2 {
  float x;
  float y;

  bool operator==( vec2 const& ) const = default;
};

template<>
struct attrib_traits<vec2> {
  inline static e_attrib_type component_type =
      e_attrib_type::float_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::vec2;
  inline static int count = 2;
};

/****************************************************************
** vec3
*****************************************************************/
struct vec3 {
  float x;
  float y;
  float z;

  bool operator==( vec3 const& ) const = default;
};

template<>
struct attrib_traits<vec3> {
  inline static e_attrib_type component_type =
      e_attrib_type::float_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::vec3;
  inline static int count = 3;
};

} // namespace gl
