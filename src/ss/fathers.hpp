/****************************************************************
**fathers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-25.
*
* Description: Game state for founding fathers.
*
*****************************************************************/
#pragma once

// Rds
#include "ss/fathers.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using FoundingFathersMap =
    refl::enum_map<e_founding_father, bool>;

using FoundingFathersPoolMap =
    refl::enum_map<e_founding_father_type,
                   base::maybe<e_founding_father>>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::FoundingFathersState,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::FoundingFathersMap, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::FoundingFathersPoolMap,
                     owned_by_cpp ){};

}
