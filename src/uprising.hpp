/****************************************************************
**uprising.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-15.
*
* Description: Implements the "Tory Uprising" mechanic.
*
*****************************************************************/
#pragma once

// rds
#include "uprising.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IGui;
struct IRand;
struct Player;
struct SS;
struct SSConst;
struct TerrainConnectivity;

enum class e_player;
enum class e_unit_type;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] bool should_attempt_uprising(
    SSConst const& ss, Player const& colonial_player,
    bool const did_deploy_ref_this_turn );

UprisingColonies find_uprising_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player colonial_player_type );

UprisingColony const& select_uprising_colony(
    IRand& rand, UprisingColonies const& uprising_colonies
                     ATTR_LIFETIMEBOUND );

std::vector<e_unit_type> generate_uprising_units( IRand& rand,
                                                  int count );

void deploy_uprising_units(
    SS& ss, UprisingColony const& uprising_colony,
    std::vector<e_unit_type> const& unit_types );

wait<> show_uprising_msg(
    SSConst const& ss, IGui& gui,
    UprisingColony const& uprising_colony );

} // namespace rn
