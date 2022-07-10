/****************************************************************
**map-updater-lua.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-07.
*
* Description: Lua extension for IMapUpdater.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

struct IMapUpdater;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::IMapUpdater, owned_by_cpp ){};
}
