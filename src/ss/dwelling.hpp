/****************************************************************
**dwelling.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Represents an indian dwelling.
*
*****************************************************************/
#pragma once

// Rds
#include "ss/dwelling.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using DwellingRelationshipMap =
    refl::enum_map<e_player, DwellingRelationship>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::Dwelling, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::DwellingTradingState,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::DwellingRelationship,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::DwellingRelationshipMap,
                     owned_by_cpp ){};

}
