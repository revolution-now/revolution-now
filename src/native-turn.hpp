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

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IRaid;
struct ITribeEvolve;
struct SS;
struct TS;

/****************************************************************
** Public API.
*****************************************************************/
wait<> natives_turn( SS& ss, TS& ts, IRaid const& raid,
                     ITribeEvolve const& tribe_evolver );

} // namespace rn
