/****************************************************************
**map-square.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-26.
*
* Description: Serializable state representing a map square.
*
*****************************************************************/
#pragma once

// Rds
#include "ss/map-square.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

void linker_dont_discard_module_gs_map_square();

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::MapSquare, owned_by_cpp ){};
}
