/****************************************************************
**land-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "sg-macros.hpp"

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( LandView );

void advance_landview_state();

struct Plane;
Plane* land_view_plane();

/****************************************************************
** Testing
*****************************************************************/
void test_land_view();

} // namespace rn
