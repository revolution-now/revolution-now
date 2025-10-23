/****************************************************************
**players.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Serializable player-related state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "players.rds.hpp"

namespace rn {

Player& player_for_player_or_die( PlayersState& players,
                                  e_player player );

Player const& player_for_player_or_die(
    PlayersState const& players, e_player player );

} // namespace rn
