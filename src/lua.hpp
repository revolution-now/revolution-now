/****************************************************************
**lua.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Lua state initialization.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace lua {
struct state;
}

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
void lua_init( lua::state& st );

} // namespace rn
