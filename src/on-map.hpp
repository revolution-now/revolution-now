/****************************************************************
**on-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-18.
*
* Description: Handles actions that need to be take in response
*              to a unit appearing on a map square (after
*              creation or moving).
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "map-updater.hpp"
#include "unit-id.hpp"

namespace rn {

struct UnitsState;

void unit_to_map_square( UnitsState&  units_state,
                         IMapUpdater& map_updater, UnitId id,
                         Coord world_square );

} // namespace rn
