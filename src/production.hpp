/****************************************************************
**production.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-31.
*
* Description: Computes what is produced by a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "production.rds.hpp"

namespace rn {

struct Colony;
struct Player;
struct TerrainState;
struct UnitsState;

// Computes everything that is produced and consumed by the
// colony in one turn, given the current state of the colony, all
// things considered. Note that some items (such as tools) can be
// both produced and consumed in the same turn, and so those will
// have to be subtracted to get the net change.
ColonyProduction production_for_colony(
    TerrainState const& terrain_state,
    UnitsState const& units_state, Player const& player,
    Colony const& colony );

} // namespace rn
