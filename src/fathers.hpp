/****************************************************************
**fathers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-26.
*
* Description: Api for querying properties of founding fathers.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "lua-enum.hpp"

// Rds
#include "fathers.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// C++ standard library
#include <string_view>
#include <vector>

namespace rn {

using FoundingFathersMap =
    refl::enum_map<e_founding_father, bool>;

/****************************************************************
** e_founding_father
*****************************************************************/
LUA_ENUM_DECL( founding_father );

std::string_view founding_father_name(
    e_founding_father father );

/****************************************************************
** e_founding_father_type
*****************************************************************/
LUA_ENUM_DECL( founding_father );

e_founding_father_type founding_father_type(
    e_founding_father father );

std::vector<e_founding_father> founding_fathers_for_type(
    e_founding_father_type type );

std::string_view founding_father_type_name(
    e_founding_father_type type );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::FoundingFathersState,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::FoundingFathersMap, owned_by_cpp ){};

}
