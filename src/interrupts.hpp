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

/****************************************************************
** Interrupts.
*****************************************************************/
struct exception_base : public std::exception {
  exception_base() = default;

  exception_base( std::string msg ) : msg_( std::move( msg ) ) {}

  char const* what() const noexcept override {
    return msg_.c_str();
  }

 private:
  std::string msg_ = "unspecified";
};

// An exception to throw when you just want to exit immediately
// via an exception (i.e., not std::abort). Mainly just for use
// during development.
struct exception_hard_exit : public exception_base {};

// Fully restart the program, closing down and destroying every-
// thing gracefully, then bringing it back up.
struct exception_restart : public exception_base {
  using exception_base::exception_base;
};

struct game_quit_interrupt : exception_base {};

struct game_load_interrupt : exception_base {
  // If this is populated then it will be loaded, otherwise the
  // load-game box will pop up.
  maybe<int> slot;
};

// Go back to the main menu. Used during the stages before a game
// is officially entered.
struct main_menu_interrupt : exception_base {};

// This is thrown when the user drags the last in-colony unit to
// the gate. Throwing an exception allows us to immediately abort
// the entire colony coroutine so that we don't have to propagate
// signals up the call stack manually. Note that when this is
// thrown, the colony object will still exist, just that it will
// have no colonists in it. So it should be ok if some code ref-
// erences the colony during the unwinding.
struct colony_abandon_interrupt : exception_base {};

// This is thrown when we want to back out all the way to the top
// of the turn loop, perhaps due to some change in the configura-
// tion of the game.
struct top_of_turn_loop : exception_base {};

// This is thrown during a turn when the user wants to enter view
// mode, i.e. the mode where the white inspection square is vis-
// ible on the map.
struct view_mode_interrupt : exception_base {
  // Use heap_value so that we can forward declare.
  base::heap_value<ViewModeOptions> options;
};

} // namespace rn
