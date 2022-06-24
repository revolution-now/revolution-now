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

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "colony.rds.hpp"

namespace rn {

using CommodityQuantityMap = refl::enum_map<e_commodity, int>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::Colony, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::CommodityQuantityMap,
                     owned_by_cpp ){};

}
