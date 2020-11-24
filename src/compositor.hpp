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
#include "aliases.hpp"
#include "coord.hpp"

namespace rn::compositor {

// In general, these sections may overlap.
enum class e_section {
  menu_bar,
  // All parts of the screen except for the menu bar.
  non_menu_bar,
  viewport,
  panel
};

// If the section is visible it will return bounds.
Opt<Rect> section( e_section section );

} // namespace rn::compositor
