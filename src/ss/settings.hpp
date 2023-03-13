/****************************************************************
**settings.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: Save-game state for game-wide settings.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "settings.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using GameOptionsMap = refl::enum_map<e_game_flag_option, bool>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::SettingsState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::GameOptions, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::GameOptionsMap, owned_by_cpp ){};

} // namespace lua
