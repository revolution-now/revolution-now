/****************************************************************
**framebuffer-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-27.
*
* Description: Unit tests for the gl/framebuffer module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gl/framebuffer.hpp"

// Testing
#include "test/mocking.hpp"
#include "test/mocks/gl/iface.hpp"

// gl
#include "src/gl/texture.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

using namespace ::mock::matchers;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gl/framebuffer] construction and bind" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().by_default().returns( GL_NO_ERROR );

  auto expect_bind = [&] {
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, 42 );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 42 );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, 41 );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
  };

  // Construction.
  mock.EXPECT__gl_GenFramebuffers( 1, Not( Null() ) )
      .sets_arg<1>( 42 );
  mock.EXPECT__gl_DeleteFramebuffers( 1, Pointee( 42 ) );
  Framebuffer fb;

  expect_bind();
  auto const binder = fb.bind();
}

TEST_CASE( "[gl/framebuffer] set_color_attachment" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().by_default().returns( GL_NO_ERROR );

  auto expect_tx_bind = [&] {
    mock.EXPECT__gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
    mock.EXPECT__gl_BindTexture( GL_TEXTURE_2D, 42 );
    mock.EXPECT__gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                 Not( Null() ) )
        .sets_arg<1>( 42 );
    mock.EXPECT__gl_BindTexture( GL_TEXTURE_2D, 41 );
    mock.EXPECT__gl_GetIntegerv( GL_TEXTURE_BINDING_2D,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
  };

  // Construction.
  mock.EXPECT__gl_GenTextures( 1, Not( Null() ) )
      .sets_arg<1>( 42 );
  expect_tx_bind();
  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  mock.EXPECT__gl_DeleteTextures( 1, Pointee( 42 ) );
  Texture tx;

  expect_tx_bind();
  gfx::size const sz{ .w = 640, .h = 360 };

  mock.EXPECT__gl_TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 640,
                              360, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                              (unsigned char*)NULL );
  tx.set_empty( sz );

  auto expect_fb_bind = [&] {
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, 42 );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 42 );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, 41 );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
  };

  mock.EXPECT__gl_GenFramebuffers( 1, Not( Null() ) )
      .sets_arg<1>( 42 );
  mock.EXPECT__gl_DeleteFramebuffers( 1, Pointee( 42 ) );
  Framebuffer fb;

  mock.EXPECT__gl_FramebufferTexture2D( GL_FRAMEBUFFER,
                                        GL_COLOR_ATTACHMENT0,
                                        GL_TEXTURE_2D, 42, 0 );
  expect_fb_bind();
  fb.set_color_attachment( tx );
}

TEST_CASE( "[gl/framebuffer] is_framebuffer_complete" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().by_default().returns( GL_NO_ERROR );

  auto expect_bind = [&] {
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, 42 );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 42 );
    mock.EXPECT__gl_BindFramebuffer( GL_FRAMEBUFFER, 41 );
    mock.EXPECT__gl_GetIntegerv( GL_FRAMEBUFFER_BINDING,
                                 Not( Null() ) )
        .sets_arg<1>( 41 );
  };

  // Construction.
  mock.EXPECT__gl_GenFramebuffers( 1, Not( Null() ) )
      .sets_arg<1>( 42 );
  mock.EXPECT__gl_DeleteFramebuffers( 1, Pointee( 42 ) );
  Framebuffer fb;

  expect_bind();
  mock.EXPECT__gl_CheckFramebufferStatus( GL_FRAMEBUFFER )
      .returns( GL_FRAMEBUFFER_COMPLETE );
  REQUIRE( fb.is_framebuffer_complete() );

  expect_bind();
  mock.EXPECT__gl_CheckFramebufferStatus( GL_FRAMEBUFFER )
      .returns( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT );
  REQUIRE_FALSE( fb.is_framebuffer_complete() );
}

} // namespace
} // namespace gl
