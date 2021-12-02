/****************************************************************
**vertex-array.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-28.
*
* Description: Unit tests for the src/gl/vertex-array.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/vertex-array.hpp"

// gl mock
#include "src/gl/iface-logger.hpp"
#include "src/gl/iface-mock.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

using namespace ::mock::matchers;

struct Vertex {
  vec3  v;
  float y;

  static consteval auto attributes() {
    return tuple{ VERTEX_ATTRIB_HOLDER( Vertex, v ),
                  VERTEX_ATTRIB_HOLDER( Vertex, y ) };
  }
};

TEST_CASE( "[vertex-array] creation" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 19 )
      .returns( GL_NO_ERROR );

  // Construct VertexArrayNonTyped.
  EXPECT_CALL( mock, gl_GenVertexArrays( 1, Not( Null() ) ) )
      .sets_arg<1>( 21 );

  // Construct vertex buffer.
  EXPECT_CALL( mock, gl_GenBuffers( 1, Not( Null() ) ) )
      .sets_arg<1>( 41 );

  // Bind vertex array.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );
  EXPECT_CALL( mock, gl_BindVertexArray( 21 ) );

  // Bind vertex buffer.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 40 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 41 ) );

  // One-time call to get max allowed attributes.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_MAX_VERTEX_ATTRIBS,
                                     Not( Null() ) ) )
      .sets_arg<1>( 10 );

  // Register attribute 0 (vec3).
  EXPECT_CALL( mock,
               gl_VertexAttribPointer(
                   /*index=*/0, /*size=*/3, /*type=*/GL_FLOAT,
                   /*normalized=*/false, /*stride=*/
                   sizeof( Vertex ),
                   /*pointer=*/(void*)0 ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 0 ) );

  // Register attribute 1 (float).
  EXPECT_CALL(
      mock,
      gl_VertexAttribPointer(
          /*index=*/1, /*size=*/1, /*type=*/GL_FLOAT,
          /*normalized=*/false, /*stride=*/sizeof( Vertex ),
          /*pointer=*/(void*)offsetof( Vertex, y ) ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 1 ) );

  // Unbind vertex buffer.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 41 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 40 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 40 );

  // Unbind vertex array.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 21 );
  EXPECT_CALL( mock, gl_BindVertexArray( 20 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );

  // Delete vertex buffer.
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 41 ) ) );

  // Delete vertex array.
  EXPECT_CALL( mock, gl_DeleteVertexArrays( 1, Pointee( 21 ) ) );

  // The call.
  VertexArray<VertexBuffer<Vertex>> arr;
}

} // namespace
} // namespace gl
