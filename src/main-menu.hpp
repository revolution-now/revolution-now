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
#include "co-combinator.hpp"
#include "waitable.hpp"

// Rds
#include "rds/main-menu.hpp"

namespace rn {

co::stream<e_main_menu_item>& main_menu_input_stream();

struct Plane;
Plane* main_menu_plane();

} // namespace rn
