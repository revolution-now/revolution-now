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

using PlayersMap = refl::enum_map<e_nation, maybe<Player>>;

using HumansMap = refl::enum_map<e_nation, bool>;

// FIXME: remove
void reset_players( PlayersState&                players_state,
                    std::vector<e_nation> const& nations,
                    base::maybe<e_nation>        human );

void set_unique_human_player( PlayersState&         players,
                              base::maybe<e_nation> nation );

Player& player_for_nation_or_die( PlayersState& players,
                                  e_nation      nation );

Player const& player_for_nation_or_die(
    PlayersState const& players, e_nation nation );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::PlayersState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::PlayersMap, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::HumansMap, owned_by_cpp ){};

} // namespace lua
