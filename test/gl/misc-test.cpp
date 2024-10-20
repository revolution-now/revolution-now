/****************************************************************
**misc-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-19.
*
* Description: Unit tests for the gl/misc module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gl/misc.hpp"

// Testing
#include "test/mocks/gl/iface.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace gl {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gl/misc] set_viewport( rect )" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );

  gfx::rect r;

  auto f = [&] { set_viewport( r ); };

  r = {};
  mock.EXPECT__gl_Viewport( 0, 0, 0, 0 );
  f();

  r = { .origin = { .x = 1, .y = 2 },
        .size   = { .w = 3, .h = 4 } };
  mock.EXPECT__gl_Viewport( 1, 2, 3, 4 );
  f();
}

TEST_CASE( "[gl/misc] set_viewport( size )" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 2 ).returns( GL_NO_ERROR );

  gfx::size s;

  auto f = [&] { set_viewport( s ); };

  s = {};
  mock.EXPECT__gl_Viewport( 0, 0, 0, 0 );
  f();

  s = { .w = 3, .h = 4 };
  mock.EXPECT__gl_Viewport( 0, 0, 3, 4 );
  f();
}

} // namespace
} // namespace gl
