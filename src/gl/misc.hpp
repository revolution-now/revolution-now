/****************************************************************
**misc.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-12.
*
* Description: Wrappers for miscellaneous OpenGL functions.
*
*****************************************************************/
#pragma once

// gl
#include "attribs.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

namespace gl {

// Clear the current buffer.
void clear( gfx::pixel color = gfx::pixel::black() );

// This defines the rectangle in physical window pixels that the
// OpenGL clip space [-1, 1] will be mapped to. Normally it is
// just set to the physical pixel dimensions of the window with
// the origin at (0, 0).
//
// This needs to be called whenever the window is resized.
void set_viewport( gfx::rect dimensions );

// This one assumes the origin is at (0, 0).
void set_viewport( gfx::size dimensions );

enum class e_gl_texture {
  tx_0,
  tx_1,
};

// This sets the texture that is implied when doing operations
// that imply a texture. We actually only need one since dif-
// ferent textures can be bound/unbound to it. I think you only
// need to care about multiple active textures if you don't want
// to bind/unbind.
void set_active_texture( e_gl_texture idx );

} // namespace gl
