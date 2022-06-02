/****************************************************************
**gs-turn.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
# Description: Turn-related save-game state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "lua-enum.hpp"

// Rds
#include "gs-turn.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

/****************************************************************
** e_season
*****************************************************************/
LUA_ENUM_DECL( season );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::TurnState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::TurnTimePoint, owned_by_cpp ){};

} // namespace lua
