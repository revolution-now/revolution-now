/****************************************************************
**tribe.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Represents one indian tribe.
*
*****************************************************************/
#pragma once

// Rds
#include "tribe.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::Tribe, owned_by_cpp ){};

}
