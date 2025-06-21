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

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using PlayersMap = refl::enum_map<e_player, maybe<Player>>;

Player& player_for_player_or_die( PlayersState& players,
                                  e_player player );

Player const& player_for_player_or_die(
    PlayersState const& players, e_player player );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::PlayersState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::PlayersMap, owned_by_cpp ){};

} // namespace lua
