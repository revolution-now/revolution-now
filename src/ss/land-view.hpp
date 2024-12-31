/****************************************************************
**gs-land-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Save-game state for the land view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "ss/land-view.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::LandViewState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::Viewport, owned_by_cpp ){};

} // namespace lua
