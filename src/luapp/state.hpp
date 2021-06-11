/****************************************************************
**state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII type for global lua state.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"

namespace lua {

/****************************************************************
** state
*****************************************************************/
// This represents a global Lua state (including all of the
// threads that it contains). It is an RAII object and thus owns
// the state.
struct state {
  // Creates an owned Lua state and sets a panic function.
  state();
  ~state() noexcept;

  // L is guaranteed to be the main thread because it is the one
  // that we get when we create the state.
  cthread main_cthread() const noexcept { return L; }

  // Do not use the state after this.
  void close();

private:
  state( state const& ) = delete;
  state( state&& )      = delete;
  state& operator=( state const& ) = delete;
  state& operator=( state&& ) = delete;

  // main thread.
  cthread L;
};

} // namespace lua
