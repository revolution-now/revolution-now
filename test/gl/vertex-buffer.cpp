/****************************************************************
**vertex-buffer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-12.
*
* Description: Unit tests for the src/gl/vertex-buffer.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/vertex-buffer.hpp"

// Testing
#include "test/mocks/gl/iface.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

using namespace ::mock::matchers;

struct Vertex {
  float x;
  float y;
};

TEST_CASE( "[vertex-buffer] interface" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 7 )
      .returns( GL_NO_ERROR );

  // Construction.
  EXPECT_CALL( mock, gl_GenBuffers( 1, Not( Null() ) ) )
      .sets_arg<1>( 42 );
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 42 ) ) );
  VertexBuffer<Vertex> buf;

  // Bind/unbind, which will happen in each of the sections be-
  // low.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 41 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 42 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 42 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 41 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 41 );

  SECTION( "bind/unbind" ) {
    // Test bind/unbind in isolation.
    auto binder = buf.bind();
  }

  SECTION( "upload_data_replace" ) {
    vector<Vertex> vertices( 10 );
    EXPECT_CALL( mock, gl_GetError() ).returns( GL_NO_ERROR );
    EXPECT_CALL(
        mock,
        gl_BufferData( GL_ARRAY_BUFFER, 10 * sizeof( Vertex ),
                       &vertices[0], GL_STATIC_DRAW ) );
    buf.upload_data_replace( vertices, e_draw_mode::stat1c );
  }

  SECTION( "upload_data_modify" ) {
    vector<Vertex> vertices( 6 );
    EXPECT_CALL( mock, gl_GetError() ).returns( GL_NO_ERROR );
    EXPECT_CALL(
        mock,
        gl_BufferSubData( GL_ARRAY_BUFFER, 2 * sizeof( Vertex ),
                          6 * sizeof( Vertex ), &vertices[0] ) );
    buf.upload_data_modify( vertices, 2 );
  }
}

} // namespace
} // namespace gl
