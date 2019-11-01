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

namespace rn {

struct Plane;
Plane* panel_plane();

// FIXME: temporary.
void mark_end_of_turn();
bool was_next_turn_button_clicked();

/****************************************************************
** Testing
*****************************************************************/
void test_panel();

} // namespace rn
