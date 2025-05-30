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

// Revolution Now
#include "src/mock/matchers.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace gl {
namespace {

using namespace std;

using ::mock::matchers::Approxf;

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

TEST_CASE( "[gl/misc] clear( pixel )" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 4 ).returns( GL_NO_ERROR );

  gfx::pixel p;

  auto f = [&] { clear( p ); };

  p = {};
  mock.EXPECT__gl_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  mock.EXPECT__gl_Clear( GL_COLOR_BUFFER_BIT |
                         GL_DEPTH_BUFFER_BIT );
  f();

  float constexpr kTolerance = 0.000001f;

  p = { .r = 10, .g = 20, .b = 30, .a = 40 };
  mock.EXPECT__gl_ClearColor( Approxf( 0.0392157f, kTolerance ),
                              Approxf( 0.0784314f, kTolerance ),
                              Approxf( 0.117647f, kTolerance ),
                              Approxf( 0.156863f, kTolerance ) );
  mock.EXPECT__gl_Clear( GL_COLOR_BUFFER_BIT |
                         GL_DEPTH_BUFFER_BIT );
  f();
}

TEST_CASE( "[gl/misc] set_active_texture" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().by_default().returns( GL_NO_ERROR );

  e_gl_texture in = {};

  auto const f = [&] { set_active_texture( in ); };

  in = e_gl_texture::tx_0;
  mock.EXPECT__gl_ActiveTexture( GL_TEXTURE0 );
  f();

  in = e_gl_texture::tx_1;
  mock.EXPECT__gl_ActiveTexture( GL_TEXTURE1 );
  f();
}

} // namespace
} // namespace gl
