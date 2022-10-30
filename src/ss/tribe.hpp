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

namespace rn {

using TribeRelationshipMap =
    refl::enum_map<e_nation, base::maybe<TribeRelationship>>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::Tribe, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::TribeRelationship, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::TribeRelationshipMap,
                     owned_by_cpp ){};

}
