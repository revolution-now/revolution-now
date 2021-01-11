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

waitable<> user_hits_eot_button();

/****************************************************************
** Testing
*****************************************************************/
void test_panel();

} // namespace rn
