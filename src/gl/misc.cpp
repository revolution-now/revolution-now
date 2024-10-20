/****************************************************************
**misc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-12.
*
* Description: Wrappers for miscellaneous OpenGL functions.
*
*****************************************************************/
#include "misc.hpp"

// gl
#include "attribs.hpp"
#include "error.hpp"
#include "iface.hpp"

using namespace std;

namespace gl {

namespace {

void clear( color floats ) {
  GL_CHECK( CALL_GL( gl_ClearColor, floats.r, floats.g, floats.b,
                     floats.a ) );
  GL_CHECK( CALL_GL(
      gl_Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );
}

}

void clear( gfx::pixel color ) {
  clear( gl::color::from_pixel( color ) );
}

void set_viewport( gfx::rect const dimensions ) {
  static constexpr int kViewportScale = 1;

  GL_CHECK( CALL_GL( gl_Viewport, dimensions.origin.x,
                     dimensions.origin.y,
                     dimensions.size.w * kViewportScale,
                     dimensions.size.h * kViewportScale ) );
}

void set_viewport( gfx::size const dimensions ) {
  set_viewport( gfx::rect{ .origin = {}, .size = dimensions } );
}

} // namespace gl
