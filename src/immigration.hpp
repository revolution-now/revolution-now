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
#include "utype.rds.hpp"

namespace rn {

struct UnitsState;
struct SettingsState;
struct ImmigrationState;
struct IGui;
class Player;

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

// void give_player_crosses_if_dock_empty(
//     UnitsState const& units_state, Player& player );
//
// int crosses_required_for_next_immigration(
//     UnitsState const&       units_state,
//     ImmigrationState const& immigration );

} // namespace rn
