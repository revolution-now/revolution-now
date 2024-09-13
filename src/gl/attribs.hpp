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
enum class e_attrib_type { int_, uint, float_ };

int to_GL( e_attrib_type type );

std::string_view to_GL_str( e_attrib_type type );

/****************************************************************
** Attribute Compound Type
*****************************************************************/
enum class e_attrib_compound_type {
  int_,
  uint,
  float_,
  vec2,
  vec3,
  vec4,
  ivec4,
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
** uint32_t
*****************************************************************/
template<>
struct attrib_traits<uint32_t> {
  inline static e_attrib_type component_type =
      e_attrib_type::uint;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::uint;
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
  static vec2 from_dsize( gfx::dsize s );

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
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 0.0f;

  static vec4 from_rect( gfx::rect r );

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
** ivec4
*****************************************************************/
struct ivec4 {
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;
  int32_t w = 0;

  static ivec4 from_rect( gfx::rect r );

  bool operator==( ivec4 const& ) const = default;
};

static_assert( sizeof( ivec4 ) == 16 );

template<>
struct attrib_traits<ivec4> {
  inline static e_attrib_type component_type =
      e_attrib_type::int_;
  inline static e_attrib_compound_type compound_type =
      e_attrib_compound_type::ivec4;
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
  static constexpr std::string_view name       = "color";
  static constexpr bool is_sumtype_alternative = false;

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
  static constexpr std::string_view name       = "vec2";
  static constexpr bool is_sumtype_alternative = false;

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gl::vec2::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gl::vec2::y,
                         offsetof( type, y ) },
  };
};

// Reflection info for struct gl::vec3.
template<>
struct traits<gl::vec3> {
  using type = gl::vec3;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name       = "vec3";
  static constexpr bool is_sumtype_alternative = false;

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gl::vec3::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gl::vec3::y,
                         offsetof( type, y ) },
      refl::StructField{ "z", &gl::vec3::z,
                         offsetof( type, z ) },
  };
};

// Reflection info for struct gl::vec4.
template<>
struct traits<gl::vec4> {
  using type = gl::vec4;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name       = "vec4";
  static constexpr bool is_sumtype_alternative = false;

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gl::vec4::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gl::vec4::y,
                         offsetof( type, y ) },
      refl::StructField{ "z", &gl::vec4::z,
                         offsetof( type, z ) },
      refl::StructField{ "w", &gl::vec4::w,
                         offsetof( type, w ) },
  };
};

// Reflection info for struct gl::ivec4.
template<>
struct traits<gl::ivec4> {
  using type = gl::ivec4;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name       = "ivec4";
  static constexpr bool is_sumtype_alternative = false;

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "x", &gl::ivec4::x,
                         offsetof( type, x ) },
      refl::StructField{ "y", &gl::ivec4::y,
                         offsetof( type, y ) },
      refl::StructField{ "z", &gl::ivec4::z,
                         offsetof( type, z ) },
      refl::StructField{ "w", &gl::ivec4::w,
                         offsetof( type, w ) },
  };
};

} // namespace refl
