/****************************************************************
**attribs.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-28.
*
* Description: Unit tests for the src/gl/attribs.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gl/attribs.hpp"

// refl
#include "refl/to-str.hpp"

// gl
#include "src/gl/iface.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

TEST_CASE( "[attribs] attribute type" ) {
  REQUIRE( to_GL( e_attrib_type::int_ ) == GL_INT );
  REQUIRE( to_GL( e_attrib_type::float_ ) == GL_FLOAT );
  REQUIRE( to_GL_str( e_attrib_type::int_ ) == "GL_INT" );
  REQUIRE( to_GL_str( e_attrib_type::float_ ) == "GL_FLOAT" );
}

TEST_CASE( "[attribs] attribute compound type" ) {
  // int
  REQUIRE( to_GL( e_attrib_compound_type::int_ ) == GL_INT );
  REQUIRE( to_GL_str( e_attrib_compound_type::int_ ) ==
           "GL_INT" );
  REQUIRE( from_GL( GL_INT ) == e_attrib_compound_type::int_ );

  // float
  REQUIRE( to_GL( e_attrib_compound_type::float_ ) == GL_FLOAT );
  REQUIRE( to_GL_str( e_attrib_compound_type::float_ ) ==
           "GL_FLOAT" );
  REQUIRE( from_GL( GL_FLOAT ) ==
           e_attrib_compound_type::float_ );

  // vec2
  REQUIRE( to_GL( e_attrib_compound_type::vec2 ) ==
           GL_FLOAT_VEC2 );
  REQUIRE( to_GL_str( e_attrib_compound_type::vec2 ) ==
           "GL_FLOAT_VEC2" );
  REQUIRE( from_GL( GL_FLOAT_VEC2 ) ==
           e_attrib_compound_type::vec2 );

  // vec3
  REQUIRE( to_GL( e_attrib_compound_type::vec3 ) ==
           GL_FLOAT_VEC3 );
  REQUIRE( to_GL_str( e_attrib_compound_type::vec3 ) ==
           "GL_FLOAT_VEC3" );
  REQUIRE( from_GL( GL_FLOAT_VEC3 ) ==
           e_attrib_compound_type::vec3 );

  // vec4
  REQUIRE( to_GL( e_attrib_compound_type::vec4 ) ==
           GL_FLOAT_VEC4 );
  REQUIRE( to_GL_str( e_attrib_compound_type::vec4 ) ==
           "GL_FLOAT_VEC4" );
  REQUIRE( from_GL( GL_FLOAT_VEC4 ) ==
           e_attrib_compound_type::vec4 );
}

template<typename T>
using Tr = attrib_traits<T>;

TEST_CASE( "[attribs] type traits" ) {
  SECTION( "int32_t" ) {
    using T = int32_t;
    REQUIRE( Tr<T>::component_type == e_attrib_type::int_ );
    REQUIRE( Tr<T>::compound_type ==
             e_attrib_compound_type::int_ );
    REQUIRE( Tr<T>::count == 1 );
  }
  SECTION( "float" ) {
    using T = float;
    REQUIRE( Tr<T>::component_type == e_attrib_type::float_ );
    REQUIRE( Tr<T>::compound_type ==
             e_attrib_compound_type::float_ );
    REQUIRE( Tr<T>::count == 1 );
  }
  SECTION( "vec2" ) {
    using T = vec2;
    REQUIRE( Tr<T>::component_type == e_attrib_type::float_ );
    REQUIRE( Tr<T>::compound_type ==
             e_attrib_compound_type::vec2 );
    REQUIRE( Tr<T>::count == 2 );
  }
  SECTION( "vec3" ) {
    using T = vec3;
    REQUIRE( Tr<T>::component_type == e_attrib_type::float_ );
    REQUIRE( Tr<T>::compound_type ==
             e_attrib_compound_type::vec3 );
    REQUIRE( Tr<T>::count == 3 );
  }
  SECTION( "vec4" ) {
    using T = vec4;
    REQUIRE( Tr<T>::component_type == e_attrib_type::float_ );
    REQUIRE( Tr<T>::compound_type ==
             e_attrib_compound_type::vec4 );
    REQUIRE( Tr<T>::count == 4 );
  }
  SECTION( "color" ) {
    using T = color;
    REQUIRE( Tr<T>::component_type == e_attrib_type::float_ );
    REQUIRE( Tr<T>::compound_type ==
             e_attrib_compound_type::vec4 );
    REQUIRE( Tr<T>::count == 4 );
  }
}

TEST_CASE( "[attribs] gfx conversion" ) {
  REQUIRE( vec2::from_point( gfx::point{ .x = 1, .y = 2 } ) ==
           vec2{ .x = 1.0, .y = 2.0 } );

  REQUIRE( color::from_pixel( gfx::pixel{
               .r = 0, .g = 255, .b = 3, .a = 4 } ) ==
           color{ .r = 0.0f,
                  .g = 255.0f / 255.0f,
                  .b = 3.0f / 255.0f,
                  .a = 4.0f / 255.0f } );
}

} // namespace
} // namespace gl
