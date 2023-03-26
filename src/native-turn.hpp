/****************************************************************
**native-turn.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Implements the logic needed to run a native
*              tribe's turn independent of any particular AI
*              model.  It just enforces game rules.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct SS;
struct TS;

wait<> natives_turn( SS& ss, TS& ts );

} // namespace rn
