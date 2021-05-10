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
#include "waitable.hpp"

namespace rn {

waitable<> show_old_world_view();

struct Plane;
Plane* old_world_plane();

} // namespace rn
