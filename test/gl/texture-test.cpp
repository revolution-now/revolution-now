/****************************************************************
**texture.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-12.
*
* Description: Unit tests for the src/gl/texture.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/texture.hpp"

// Testing
#include "test/mocks/gl/iface.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;

using namespace ::mock::matchers;

TEST_CASE( "[texture] construct then set image" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 7 ).returns( GL_NO_ERROR );

  auto expect_bind = [&] {
    mock.EXPECT__gl_GetError().times( 5 ).returns( GL_NO_ERROR );
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
  expect_bind();
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

  SECTION( "set_image" ) {
    expect_bind();
    gfx::image img =
        gfx::new_empty_image( gfx::size{ .w = 7, .h = 5 } );

    mock.EXPECT__gl_TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 7, 5,
                                0, GL_RGBA, GL_UNSIGNED_BYTE,
                                img.data() );
    tx.set_image( img );
  }
}

TEST_CASE( "[texture] construct with image" ) {
  gl::MockOpenGL mock;

  mock.EXPECT__gl_GetError().times( 7 ).returns( GL_NO_ERROR );

  auto expect_bind = [&] {
    mock.EXPECT__gl_GetError().times( 5 ).returns( GL_NO_ERROR );
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
  expect_bind();
  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

  mock.EXPECT__gl_TexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  mock.EXPECT__gl_DeleteTextures( 1, Pointee( 42 ) );
  expect_bind();
  gfx::image img =
      gfx::new_empty_image( gfx::size{ .w = 7, .h = 5 } );

  mock.EXPECT__gl_TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 7, 5, 0,
                              GL_RGBA, GL_UNSIGNED_BYTE,
                              img.data() );
  Texture tx( img );
}

} // namespace
} // namespace gl
