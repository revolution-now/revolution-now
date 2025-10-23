/****************************************************************
**colony.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-15.
*
* Description: Data structure representing a Colony.
*
*****************************************************************/
#pragma once

// rds
#include "ss/colony.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

std::vector<UnitId> colony_units_all( Colony const& colony );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Colony, owned_by_cpp ){};
}
