/****************************************************************
**uniform.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-02.
*
* Description: Unit tests for the src/gl/uniform.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/uniform.hpp"

// Testing
#include "test/mocks/gl/iface.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;
using namespace ::mock::matchers;

using ::base::invalid;
using ::base::valid;

TEST_CASE( "[uniform] creation/set float" ) {
  gl::MockOpenGL mock;

  // Construct UniformNonTyped.
  mock.EXPECT__gl_GetError().returns( GL_NO_ERROR );
  mock.EXPECT__gl_GetUniformLocation(
          7, Eq<string>( "some_uniform" ) )
      .returns( 88 );

  Uniform<float> uf( 7, "some_uniform" );

  // Set float failure.
  {
    gl::MockOpenGL mock2;
    mock2.EXPECT__gl_UseProgram( 7 );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    mock2.EXPECT__gl_Uniform1f( 88, 5.5 );
    mock2.EXPECT__gl_GetError().returns( GL_INVALID_OPERATION );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    REQUIRE(
        uf.try_set( 5.5 ) ==
        invalid<string>( "failed to set uniform as float" ) );
  }

  // Set float success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform1f( 88, 5.5 );
  REQUIRE( uf.try_set( 5.5 ) == valid );

  // Set float again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( 5.5 ) == valid );

  // Set float different value, success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform1f( 88, 5.6 );
  // This time, set with operator=.
  uf = 5.6;

  // Set float again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( 5.6 ) == valid );
}

TEST_CASE( "[uniform] creation/set long" ) {
  gl::MockOpenGL mock;

  // Construct UniformNonTyped.
  mock.EXPECT__gl_GetError().returns( GL_NO_ERROR );
  mock.EXPECT__gl_GetUniformLocation(
          7, Eq<string>( "some_uniform" ) )
      .returns( 88 );

  Uniform<long> uf( 7, "some_uniform" );

  // Set long failure.
  {
    gl::MockOpenGL mock2;
    mock2.EXPECT__gl_UseProgram( 7 );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    mock2.EXPECT__gl_Uniform1i( 88, 5 );
    mock2.EXPECT__gl_GetError().returns( GL_INVALID_OPERATION );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    REQUIRE(
        uf.try_set( 5 ) ==
        invalid<string>( "failed to set uniform as long" ) );
  }

  // Set long success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform1i( 88, 5 );
  REQUIRE( uf.try_set( 5 ) == valid );

  // Set long again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( 5 ) == valid );

  // Set long different value, success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform1i( 88, 6 );
  REQUIRE( uf.try_set( 6 ) == valid );

  // Set long again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( 6 ) == valid );
}

TEST_CASE( "[uniform] creation/set bool" ) {
  gl::MockOpenGL mock;

  // Construct UniformNonTyped.
  mock.EXPECT__gl_GetError().returns( GL_NO_ERROR );
  mock.EXPECT__gl_GetUniformLocation(
          7, Eq<string>( "some_uniform" ) )
      .returns( 88 );

  Uniform<bool> uf( 7, "some_uniform" );

  // Set bool failure.
  {
    gl::MockOpenGL mock2;
    mock2.EXPECT__gl_UseProgram( 7 );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    mock2.EXPECT__gl_Uniform1i( 88, true );
    mock2.EXPECT__gl_GetError().returns( GL_INVALID_OPERATION );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    REQUIRE(
        uf.try_set( true ) ==
        invalid<string>( "failed to set uniform as bool" ) );
  }

  // Set bool success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform1i( 88, true );
  REQUIRE( uf.try_set( true ) == valid );

  // Set bool again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( true ) == valid );

  // Set bool different value, success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform1i( 88, false );
  REQUIRE( uf.try_set( false ) == valid );

  // Set bool again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( false ) == valid );
}

TEST_CASE( "[uniform] creation/set vec2" ) {
  gl::MockOpenGL mock;

  // Construct UniformNonTyped.
  mock.EXPECT__gl_GetError().returns( GL_NO_ERROR );
  mock.EXPECT__gl_GetUniformLocation(
          7, Eq<string>( "some_uniform" ) )
      .returns( 88 );

  Uniform<vec2> uf( 7, "some_uniform" );

  // Set vec2 failure.
  {
    gl::MockOpenGL mock2;
    mock2.EXPECT__gl_UseProgram( 7 );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    mock2.EXPECT__gl_Uniform2f( 88, 4.5, 5.6 );
    mock2.EXPECT__gl_GetError().returns( GL_INVALID_OPERATION );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    REQUIRE(
        uf.try_set( vec2{ .x = 4.5, .y = 5.6 } ) ==
        invalid<string>( "failed to set uniform as vec2" ) );
  }

  // Set vec2 success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform2f( 88, 4.5, 5.6 );
  REQUIRE( uf.try_set( vec2{ .x = 4.5, .y = 5.6 } ) == valid );

  // Set vec2 again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( vec2{ .x = 4.5, .y = 5.6 } ) == valid );

  // Set vec2 different value, success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform2f( 88, 6.7, 7.8 );
  REQUIRE( uf.try_set( vec2{ .x = 6.7, .y = 7.8 } ) == valid );

  // Set vec2 again (no GL calls since it should use the cached
  // value).
  REQUIRE( uf.try_set( vec2{ .x = 6.7, .y = 7.8 } ) == valid );
}

TEST_CASE( "[uniform] creation/set array of ivec3" ) {
  gl::MockOpenGL mock;

  // Construct UniformNonTyped.
  mock.EXPECT__gl_GetError().returns( GL_NO_ERROR );
  mock.EXPECT__gl_GetUniformLocation(
          7, Eq<string>( "some_uniform" ) )
      .returns( 88 );

  Uniform<span<ivec3 const>> uf( 7, "some_uniform" );

  static ivec3 const values[3] = {
    { .x = 5, .y = 6, .z = 7 },
    { .x = 6, .y = 7, .z = 8 },
    { .x = 7, .y = 8, .z = 9 },
  };

  // Set ivec3 failure.
  {
    gl::MockOpenGL mock2;
    mock2.EXPECT__gl_UseProgram( 7 );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    mock2.EXPECT__gl_Uniform3iv( 88, 3, &values[0].x );
    mock2.EXPECT__gl_GetError().returns( GL_INVALID_OPERATION );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    REQUIRE( uf.try_set( values ) ==
             invalid<string>(
                 "failed to set uniform as array of ivec3" ) );
  }

  // Set ivec3 success (empty).
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform3iv( 88, 0, _ );
  REQUIRE( uf.try_set( span<ivec3 const>{} ) == valid );

  // Set ivec3 success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform3iv( 88, 3, &values[0].x );
  REQUIRE( uf.try_set( values ) == valid );

  // Set ivec3 again (cache not used for this type).
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform3iv( 88, 3, &values[0].x );
  REQUIRE( uf.try_set( values ) == valid );
}

TEST_CASE( "[uniform] creation/set array of ivec4" ) {
  gl::MockOpenGL mock;

  // Construct UniformNonTyped.
  mock.EXPECT__gl_GetError().returns( GL_NO_ERROR );
  mock.EXPECT__gl_GetUniformLocation(
          7, Eq<string>( "some_uniform" ) )
      .returns( 88 );

  Uniform<span<ivec4 const>> uf( 7, "some_uniform" );

  static ivec4 const values[3] = {
    { .x = 5, .y = 6, .z = 7, .w = 8 },
    { .x = 6, .y = 7, .z = 8, .w = 9 },
    { .x = 7, .y = 8, .z = 9, .w = 10 },
  };

  // Set ivec4 failure.
  {
    gl::MockOpenGL mock2;
    mock2.EXPECT__gl_UseProgram( 7 );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    mock2.EXPECT__gl_Uniform4iv( 88, 3, &values[0].x );
    mock2.EXPECT__gl_GetError().returns( GL_INVALID_OPERATION );
    mock2.EXPECT__gl_GetError().returns( GL_NO_ERROR );
    REQUIRE( uf.try_set( values ) ==
             invalid<string>(
                 "failed to set uniform as array of ivec4" ) );
  }

  // Set ivec4 success (empty).
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform4iv( 88, 0, _ );
  REQUIRE( uf.try_set( span<ivec4 const>{} ) == valid );

  // Set ivec4 success.
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform4iv( 88, 3, &values[0].x );
  REQUIRE( uf.try_set( values ) == valid );

  // Set ivec4 again (cache not used for this type).
  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );
  mock.EXPECT__gl_UseProgram( 7 );
  mock.EXPECT__gl_Uniform4iv( 88, 3, &values[0].x );
  REQUIRE( uf.try_set( values ) == valid );
}

} // namespace
} // namespace gl
