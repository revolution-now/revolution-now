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

// Rds
#include "player.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

void linker_dont_discard_module_player();

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Player, owned_by_cpp ){};
}
