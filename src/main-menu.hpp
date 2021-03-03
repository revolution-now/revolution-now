/****************************************************************
**main-menu.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "waitable.hpp"

// Rnl
#include "rnl/main-menu.hpp"

namespace rn {

// When this function returns a value, that value will be reset,
// so another call immediately after will yield no result.
waitable<e_main_menu_item> next_main_menu_item();

struct Plane;
Plane* main_menu_plane();

} // namespace rn
