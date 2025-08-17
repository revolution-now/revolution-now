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
struct SS;
struct TS;

/****************************************************************
** Public API.
*****************************************************************/
void do_keep_playing_after_winning( SS& ss, TS& ts );

void do_keep_playing_after_timeout( SS& ss, TS& ts );

wait<e_keep_playing> ask_keep_playing( IGui& gui );

wait<> check_time_up( SS& ss, TS& ts );

} // namespace rn
