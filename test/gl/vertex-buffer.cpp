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

using ::mock::_;

struct Vertex {
  float x;
  float y;
};

TEST_CASE( "[vertex-buffer] some test" ) {
  gl::MockOpenGL mock;
  gl::set_global_gl_implementation( &mock );
  EXPECT_MULTIPLE_CALLS( mock, gl_GetError() )
      .returns( GL_NO_ERROR );

}

} // namespace
} // namespace gl
