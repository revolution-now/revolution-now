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

// Revolution Now
#include "maybe.hpp"

// base
#include "base/heap-value.hpp"

// C++ standard library
#include <exception>

namespace rn {

struct ViewModeOptions;

enum class e_nation;

/****************************************************************
** Interrupts.
*****************************************************************/
struct game_quit_interrupt : std::exception {};

struct game_load_interrupt : std::exception {
  // If this is populated then it will be loaded, otherwise the
  // load-game box will pop up.
  maybe<int> slot;
};

// Go back to the main menu. Used during the stages before a game
// is officially entered.
struct main_menu_interrupt : std::exception {};

// This is thrown when the user drags the last in-colony unit to
// the gate. Throwing an exception allows us to immediately abort
// the entire colony coroutine so that we don't have to propagate
// signals up the call stack manually. Note that when this is
// thrown, the colony object will still exist, just that it will
// have no colonists in it. So it should be ok if some code ref-
// erences the colony during the unwinding.
struct colony_abandon_interrupt : std::exception {};

// This is thrown when we want to back out all the way to the top
// of the turn loop, perhaps due to some change in the configura-
// tion of the game.
struct top_of_turn_loop : std::exception {};

// This is thrown during a turn when the user wants to enter view
// mode, i.e. the mode where the white inspection square is vis-
// ible on the map.
struct view_mode_interrupt : std::exception {
  // Use heap_value so that we can forward declare.
  base::heap_value<ViewModeOptions> options;
};

// No turning back from here.
struct declare_independence_interrupt : std::exception {
  e_nation nation = {};
};

} // namespace rn
