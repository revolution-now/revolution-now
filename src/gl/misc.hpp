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

// Clear the current buffer.
void clear( color c );

// This defines the rectangle in physical window pixels that the
// OpenGL clip space [-1, 1] will be mapped to. Normally it is
// just set to the physical pixel dimensions of the window with
// the origin at (0, 0).
//
// This needs to be called whenever the window is resized.
void set_viewport( gfx::rect dimensions );

// This one assumes the origin is at (0, 0).
void set_viewport( gfx::size dimensions );

} // namespace gl
