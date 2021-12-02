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

// gl
#include "src/gl/iface.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

TEST_CASE( "[attribs] attribute type" ) {
  REQUIRE( to_GL( e_attrib_type::float_ ) == GL_FLOAT );
  REQUIRE( to_GL_str( e_attrib_type::float_ ) == "GL_FLOAT" );
}

TEST_CASE( "[attribs] attribute compound type" ) {
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
}

} // namespace
} // namespace gl
