/****************************************************************
**sons-of-liberty.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-19.
*
# Description: Representation for state relating to SoL.
*
*****************************************************************/
#pragma once

// Rds
#include "ss/sons-of-liberty.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::SonsOfLiberty, owned_by_cpp ){};

}
