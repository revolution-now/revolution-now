/****************************************************************
**native-unit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-21.
*
* Description: Represents native units (various kinds of braves).
*
*****************************************************************/
#pragma once

// Rds
#include "native-unit.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::NativeUnit, owned_by_cpp ){};

}
