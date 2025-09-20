/****************************************************************
**game-end.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Handles general things related to ending the game.
*
*****************************************************************/
#pragma once

// rds
#include "game-end.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct IGui;
struct Player;
struct SS;
struct SSConst;
struct TS;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] bool check_time_up( SSConst const& ss );

wait<e_game_end> do_time_up( SS& ss, IGui& gui );

wait<e_game_end> check_for_ref_win( SS& ss, TS& ts,
                                    Player const& ref_player );

// Returns true if the game has stopped and we should return to
// the main menu.
wait<e_game_end> check_for_ref_forfeight( SS& ss, TS& ts,
                                          Player& ref_player );

} // namespace rn
