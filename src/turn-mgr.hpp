/****************************************************************
**turn-mgr.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-20.
*
* Description: Helpers for turn processing.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "maybe.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct SSConst;

enum class e_player;

/****************************************************************
** Public API.
*****************************************************************/
// Finds the first player to move each turn. The result will only
// be `nothing` if there are no players in the game.
maybe<e_player> find_first_player_to_move( SSConst const& ss );

// Given the current player that just moved, finds the next one
// to move this turn. The result will be `nothing` if either
// we've reached the end of the player list or there are no na-
// tions present.
maybe<e_player> find_next_player_to_move(
    SSConst const& ss, e_player const curr_player );

} // namespace rn
