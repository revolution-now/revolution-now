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
#include "enum.hpp"

namespace rn::compositor {

// In general, these sections may overlap.
enum class e_(
    section,  // e_section
    menu_bar, //
    // All parts of the screen except for the menu bar.
    non_menu_bar, //
    viewport,     //
    panel         //
);

Rect section( e_section section );

} // namespace rn::compositor
