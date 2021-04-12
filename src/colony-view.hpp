/****************************************************************
**colony-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-05.
*
* Description: The view that appears when clicking on a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "id.hpp"
#include "waitable.hpp"

namespace rn {

waitable<> show_colony_view( ColonyId id );

struct Plane;
Plane* colony_plane();

} // namespace rn
