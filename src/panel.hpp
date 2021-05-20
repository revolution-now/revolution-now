/****************************************************************
**panel.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-11-01.
*
* Description: The side panel on land view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "waitable.hpp"

namespace rn {

struct Plane;
Plane* panel_plane();

waitable<> wait_for_eot_button_click();

} // namespace rn
