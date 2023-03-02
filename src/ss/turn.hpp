/****************************************************************
**turn.hpp
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

// Rds
#include "ss/turn.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::TurnState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::TurnTimePoint, owned_by_cpp ){};

} // namespace lua
