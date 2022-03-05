/****************************************************************
**index-buffer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-05.
*
* Description: Unit tests for the src/gl/index-buffer.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/index-buffer.hpp"

// gl mock
#include "src/gl/iface-mock.hpp"

// C++ stanard library
#include <array>

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

using namespace ::mock::matchers;

TEST_CASE( "[gl/index-buffer] construction" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 13 )
      .returns( GL_NO_ERROR );

  // Construction.
  EXPECT_CALL( mock, gl_GenBuffers( 1, Not( Null() ) ) )
      .sets_arg<1>( 42 );
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 42 ) ) );
  // Bind/unbind.
  EXPECT_CALL( mock,
               gl_GetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING,
                               Not( Null() ) ) )
      .sets_arg<1>( 41 );
  EXPECT_CALL( mock,
               gl_BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 42 ) );
  EXPECT_CALL( mock,
               gl_GetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING,
                               Not( Null() ) ) )
      .sets_arg<1>( 42 );
  EXPECT_CALL( mock,
               gl_BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 41 ) );
  EXPECT_CALL( mock,
               gl_GetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING,
                               Not( Null() ) ) )
      .sets_arg<1>( 41 );
  static array<unsigned int, 6> indices{
      0, 1, 3, // first triangle.
      1, 2, 3, // second triangle.
  };
  EXPECT_CALL(
      mock,
      gl_BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ),
                     Not( Null() ), GL_STATIC_DRAW ) );
  IndexBuffer buf;

  // Bind/unbind, which happens in each of the sections below.
  EXPECT_CALL( mock,
               gl_GetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING,
                               Not( Null() ) ) )
      .sets_arg<1>( 41 );
  EXPECT_CALL( mock,
               gl_BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 42 ) );
  EXPECT_CALL( mock,
               gl_GetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING,
                               Not( Null() ) ) )
      .sets_arg<1>( 42 );
  EXPECT_CALL( mock,
               gl_BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 41 ) );
  EXPECT_CALL( mock,
               gl_GetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING,
                               Not( Null() ) ) )
      .sets_arg<1>( 41 );

  SECTION( "bind/unbind" ) {
    // Test bind/unbind in isolation.
    auto binder = buf.bind();
  }
}

} // namespace
} // namespace gl
