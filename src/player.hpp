/****************************************************************
**player.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fathers.hpp"
#include "nation.hpp"

// Rds
#include "player.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Abseil
#include "absl/types/span.h"

namespace rn {

struct PlayersState;

Player& player_for_nation( PlayersState& players_state,
                           e_nation      nation );

Player const& player_for_nation(
    PlayersState const& players_state, e_nation nation );

// FIXME: deprecated
Player& player_for_nation( e_nation nation );

void linker_dont_discard_module_player();

using FoundingFathersMap =
    refl::enum_map<e_founding_father, bool>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::Player, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::FoundingFathersMap, owned_by_cpp ){};

}
