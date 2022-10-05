/****************************************************************
**fog-square.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Represents a player's view of a map square.
*
*****************************************************************/
#pragma once

// Rds
#include "fog-square.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( rn::FogSquare, owned_by_cpp ){};
} // namespace lua
