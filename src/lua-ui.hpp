/****************************************************************
**lua-ui.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-19.
*
* Description: Exposes some UI functions to Lua.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "waitable.hpp"

namespace rn {

void linker_dont_discard_module_lua_ui();

waitable<> lua_ui_test();

} // namespace rn
