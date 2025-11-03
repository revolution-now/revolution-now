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
// This runs the full init routine including loading all modules,
// which is expensive.
void lua_init( lua::state& st );

// This is mainly for unit tests where we want to run the startup
// routines but nothing else, for efficiency.
void run_lua_startup_routines( lua::state& st );

} // namespace rn
