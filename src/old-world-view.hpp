/****************************************************************
**old-world-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the Old World port view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "id.hpp"
#include "wait.hpp"

namespace rn {

wait<> show_old_world_view();

void old_world_view_set_selected_unit( UnitId id );

struct Plane;
Plane* old_world_plane();

} // namespace rn
