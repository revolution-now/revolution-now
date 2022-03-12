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

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

// refl
#include "refl/ext.hpp"

// C++ standard library
#include <string_view>

namespace gl {

/****************************************************************
** Attribute Type
*****************************************************************/
enum class e_attrib_type { int_, float_ };

int to_GL( e_attrib_type type );

std::string_view to_GL_str( e_attrib_type type );

/****************************************************************
** Attribute Compound Type
*****************************************************************/
enum class e_attrib_compound_type {
  int_,
  float_,
  vec2,
  vec3,
  vec4
};

int to_GL( e_attrib_compound_type type );

e_attrib_compound_type from_GL( int type );

std::string_view to_GL_str( e_attrib_compound_type type );

/****************************************************************
** Attribute Type Traits
*****************************************************************/
template<typename T>
struct attrib_traits;

/****************************************************************
** int32_t
*****************************************************************/
template<>
struct attrib_traits<int32_t> {
  inline static e_attrib_type component_type =
      e_attrib_type::int_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::int_;
  inline static int count = 1;
};

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
  float x = 0.0f;
  float y = 0.0f;

  static vec2 from_point( gfx::point p );
  static vec2 from_size( gfx::size s );

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
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

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

/****************************************************************
** vec4
*****************************************************************/
struct vec4 {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;

  bool operator==( vec4 const& ) const = default;
};

template<>
struct attrib_traits<vec4> {
  inline static e_attrib_type component_type =
      e_attrib_type::float_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::vec4;
  inline static int count = 4;
};

/****************************************************************
** color
*****************************************************************/
struct color {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;

  static color from_pixel( gfx::pixel p );

  bool operator==( color const& ) const = default;
};

template<>
struct attrib_traits<color> {
  inline static e_attrib_type component_type =
      e_attrib_type::float_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::vec4;
  inline static int count = 4;
};

} // namespace gl

/****************************************************************
** Reflection
*****************************************************************/
namespace refl {

// Reflection info for struct gl::color.
template<>
struct traits<gl::color> {
  using type = gl::color;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name = "color";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "r", &gl::color::r,
                         offsetof( type, r ) },
      refl::StructField{ "g", &gl::color::g,
                         offsetof( type, g ) },
      refl::StructField{ "b", &gl::color::b,
                         offsetof( type, b ) },
      refl::StructField{ "a", &gl::color::a,
                         offsetof( type, a ) },
  };
};

// Reflection info for struct gl::vec2.
template<>
struct traits<gl::vec2> {
  using type = gl::vec2;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name = "vec2";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gl::vec2::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gl::vec2::y,
                         offsetof( type, y ) },
  };
};

} // namespace refl
