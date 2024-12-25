/****************************************************************
**compositor.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-04.
*
* Description: Coordinates layout of elements on screen.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Rds
#include "compositor.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn::compositor {

// If the section is visible it will return bounds.
maybe<Rect> section( gfx::rect logical_screen_rect,
                     e_section section );

// Return the region that is not the given section, if possible.
// If the section in question is not visible then the entire
// screen area will be returned. The way this can fail is if the
// inverted area cannot be represented by a single rectangle.
maybe<Rect> section_inverted( gfx::rect logical_screen_rect,
                              e_section section );

// Rotates the position of the console clockwise.
void rotate_console();

void set_console_size( double percent );

} // namespace rn::compositor
