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
#include "wait.hpp"

namespace rn {

struct Planes;

wait<> lua_ui_test( Planes& planes );

} // namespace rn
