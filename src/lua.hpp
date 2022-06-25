/****************************************************************
**lua.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Interface to lua.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace lua {

struct state;

}

namespace rn {

struct RootState;

/****************************************************************
** Lua State
*****************************************************************/
lua::state& lua_global_state();

/****************************************************************
** Lua Modules
*****************************************************************/
void run_lua_startup_routines();
void load_lua_modules();

void lua_reload( RootState& root_state );

} // namespace rn
