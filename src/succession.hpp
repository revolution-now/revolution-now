/****************************************************************
**succession.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Implements the War of Succession.
*
*****************************************************************/
#pragma once

// rds
#include "succession.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Player;
struct SS;
struct TS;
struct SSConst;

/****************************************************************
** War of Succession.
*****************************************************************/
bool should_do_war_of_succession( SSConst const& ss,
                                  Player const& player );

WarOfSuccessionNations select_players_for_war_of_succession(
    SSConst const& ss );

WarOfSuccessionPlan war_of_succession_plan(
    SSConst const& ss, WarOfSuccessionNations const& nations );

void do_war_of_succession( SS& ss, TS& ts, Player const& player,
                           WarOfSuccessionPlan const& plan );

wait<> do_war_of_succession_ui_seq(
    TS& ts, WarOfSuccessionPlan const& plan );

} // namespace rn
