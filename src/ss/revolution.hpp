/****************************************************************
**revolution.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-06.
*
* Description: Holds state related to the war of independence.
*
*****************************************************************/
#pragma once

#include "revolution.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::ExpeditionaryForce, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::RevolutionState, owned_by_cpp ){};

} // namespace lua
