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
#include "string.hpp"
#include "table.hpp"
#include "thread.hpp"

// C++ standard library
#include <string_view>

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

  // Do not use the state after this.
  void close();

  /**************************************************************
  ** Threads
  ***************************************************************/
  // L is guaranteed to be the main thread because it is the one
  // that we get when we create the state.
  cthread main_cthread() const noexcept { return L; }

  rthread main_rthread() const noexcept;

  /**************************************************************
  ** Strings
  ***************************************************************/
  rstring str( std::string_view sv ) noexcept;

  /**************************************************************
  ** Tables
  ***************************************************************/
  table global_table() noexcept;
  table new_table() noexcept;

  template<typename U>
  auto operator[]( U&& idx ) noexcept {
    return global_table()[std::forward<U>( idx )];
  }

private:
  state( state const& ) = delete;
  state( state&& )      = delete;
  state& operator=( state const& ) = delete;
  state& operator=( state&& ) = delete;

  // main thread.
  cthread L;
};

// Cannot push an entire global state. You can push a thread
// though (rthread).
void push( cthread, state const& ) = delete;

} // namespace lua
