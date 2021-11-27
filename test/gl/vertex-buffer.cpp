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

// gl mock
#include "src/gl/iface-mock.hpp"

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

TEST_CASE( "[vertex-buffer] creation" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 2 )
      .returns( GL_NO_ERROR );

  EXPECT_CALL( mock, gl_GenBuffers( 1, _ ) ).sets_arg<1>( 42 );
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 42 ) ) );
  VertexBuffer<Vertex> buf;
}

} // namespace
} // namespace gl
