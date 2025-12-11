/****************************************************************
**construction.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-20.
*
* Description: All things related to colony construction.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "construction.rds.hpp"

// Revolution Now
#include "maybe.hpp"
#include "wait.hpp"

// C++ standard library
#include <string>

namespace rn {

struct IGui;
struct Colony;
struct Player;
struct SSConst;
struct TS;

std::string construction_name(
    Construction const& construction );

// The outter maybe is when the user just escapes, the inner one
// is for when they select no production.
wait<> select_colony_construction( SSConst const& ss, TS& ts,
                                   Colony& colony );

// Computes the cost of rushing the construction project given
// the hammers and tools already in the colony. If there is
// nothing being built or if the project is a building that al-
// ready exists in the colony then it returns nothing.
maybe<RushConstruction> rush_construction_cost(
    SSConst const& ss, Colony const& colony );

// Opens the box that opens when the user asks to rush completion
// of a construction project.
wait<> rush_construction_prompt(
    Player& player, Colony& colony, IGui& gui,
    RushConstruction const& invoice );

// In the OG the number of wagon trains one can build at a given
// time is limited. This will perform that check.
[[nodiscard]] bool wagon_train_limit_exceeded(
    SSConst const& ss, Player const& player );

} // namespace rn
