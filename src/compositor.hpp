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
#include "coord.hpp"

// Rds
#include "rds/compositor.hpp"

namespace rn::compositor {

// If the section is visible it will return bounds.
maybe<Rect> section( e_section section );

// Return the region that is not the given section, if possible.
// If the section in question is not visible then the entire
// screen area will be returned. The way this can fail is if the
// inverted area cannot be represented by a single rectangle.
maybe<Rect> section_inverted( e_section section );

// Zero effectively removes the console.
void set_console_height( H height );

} // namespace rn::compositor
