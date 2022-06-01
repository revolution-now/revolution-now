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

// Revolution Now
#include "wait.hpp"

// Rds
#include "immigration.rds.hpp"
#include "nation.rds.hpp"
#include "utype.rds.hpp"

namespace rn {

struct UnitsState;
struct SettingsState;
struct ImmigrationState;
struct IGui;
struct Player;

// Presents the user with the three unit types that are currently
// in the immigration pool and asks to choose one. The player can
// also press escape in which case nothing will be returned.
wait<maybe<int>> ask_player_to_choose_immigrant(
    IGui& gui, ImmigrationState const& immigration,
    std::string msg );

// Remove one unit type ("immigrant") from the pool and replace
// it (in the same slot) with replacement.  n must be [0,3).
e_unit_type take_immigrant_from_pool(
    ImmigrationState& immigration, int n,
    e_unit_type replacement );

// Randomly selects the next unit type for the immigration pool.
e_unit_type pick_next_unit_for_pool(
    Player const& player, SettingsState const& settings );

// This is not cheap to compute, so should be computed only when
// needed.
CrossesCalculation compute_crosses(
    UnitsState const& units_state, e_nation nation );

// This is called each turn to accumulate some crosses based on
// production and dock bonus/malus. Note that the total colonies'
// cross production should already include the bonuses based on
// William Penn and Sons of Liberty membership.
void add_player_crosses( Player& player,
                         int     total_colonies_cross_production,
                         int     dock_crosses_bonus );

} // namespace rn
