/****************************************************************
**scratch.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: "Scratch" state for querying Lua behavior on value
*              value types.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace lua {

struct state;

// The Lua state returned here should ONLY be used to do simple
// things such as compare value types. It should not be used to
// hold any objects and nothing in its state should be changed.
state& scratch_state();

} // namespace lua
