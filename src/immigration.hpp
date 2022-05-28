/****************************************************************
**immigration.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-27.
*
* Description: All things immigration.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "old-world-state.rds.hpp"
#include "utype.rds.hpp"

namespace rn {

struct UnitsState;
struct Player;

void give_player_crosses_if_dock_empty(
    UnitsState const& units_state, Player& player );

int crosses_required_for_next_immigration(
    UnitsState const&       units_state,
    ImmigrationState const& immigration );

e_unit_type pick_next_unit_for_pool();

e_unit_type take_immigrant_and_replace(
    int slot, e_unit_type replacement );

} // namespace rn
