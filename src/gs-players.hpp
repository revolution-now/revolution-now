/****************************************************************
**gs-players.hpp
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
#include "gs-players.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using PlayersMap = std::unordered_map<e_nation, Player>;
static_assert( std::is_same_v<
               PlayersMap, decltype( PlayersState::players )> );

void reset_players( PlayersState&                players_state,
                    std::vector<e_nation> const& nations );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::PlayersState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::PlayersMap, owned_by_cpp ){};

} // namespace lua
