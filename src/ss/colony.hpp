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
#include "ss/colony.rds.hpp"

namespace rn {

using CommodityQuantityMap = refl::enum_map<e_commodity, int>;
using ColonyBuildingsMap =
    refl::enum_map<e_colony_building, bool>;

std::vector<UnitId> colony_units_all( Colony const& colony );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::Colony, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::CommodityQuantityMap,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::ColonyBuildingsMap, owned_by_cpp ){};

}
