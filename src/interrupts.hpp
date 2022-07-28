/****************************************************************
**interrupts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: Exceptions that can be thrown to change game
*              state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <exception>

namespace rn {

struct game_quit_interrupt : std::exception {};
struct game_load_interrupt : std::exception {};

// This is thrown when the user drags the last in-colony unit to
// the gate. Throwing an exception allows us to immediately abort
// the entire colony coroutine so that we don't have to propagate
// signals up the call stack manually. Note that when this is
// thrown, the colony object will still exist, just that it will
// have no colonists in it. So it should be ok if some code ref-
// erences the colony during the unwinding.
struct colony_abandon_interrupt : std::exception {};

} // namespace rn
