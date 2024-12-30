/****************************************************************
**app-ctrl.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Handles the top-level game state machines.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IEngine;
struct Planes;

// This is the top-level coroutine in the program which calls
// into all of the other coroutines. Overall, one can think of
// this program as an event loop that spins, taking user input,
// until this top-level await is finished.
wait<> revolution_now( IEngine& engine, Planes& planes );

} // namespace rn
