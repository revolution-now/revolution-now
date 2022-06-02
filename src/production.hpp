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
#include "commodity.rds.hpp"
#include "production.rds.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct Colony;
struct PlayersState;
struct UnitsState;

struct ColonyProduction {
  refl::enum_map<e_colony_product, int> produced;
  refl::enum_map<e_commodity, int>      consumed;
};

// Computes everything that is produced and consumed by the
// colony in one turn, given the current state of the colony, all
// things considered. Note that some items (such as tools) can be
// both produced and consumed in the same turn, and so those will
// have to be subtracted to get the net change.
ColonyProduction production_for_colony(
    UnitsState const&   units_state,
    PlayersState const& players_state, Colony const& colony );

} // namespace rn
