/****************************************************************
**harbor-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "unit-id.hpp"
#include "wait.hpp"

namespace rn {

wait<> show_harbor_view();

void harbor_view_set_selected_unit( UnitId id );

struct Plane;
Plane* harbor_plane();

} // namespace rn
