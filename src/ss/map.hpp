/****************************************************************
**map.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-30.
*
* Description: Save-game state for stuff that is associated with
*              the map but not used for rendering.
*
*****************************************************************/
#pragma once

// Rds
#include "map.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( rn::MapState, owned_by_cpp ){};
} // namespace lua
