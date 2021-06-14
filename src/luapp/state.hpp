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
#include "rstring.hpp"
#include "rtable.hpp"
#include "rthread.hpp"

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

  bool alive() const { return L != nullptr; }

private:
  // main thread.
  cthread L;

public:
  /**************************************************************
  ** Threads
  ***************************************************************/
  struct Thread {
    Thread( cthread cth );

    // This is guaranteed to be the main thread because it is the
    // one that we get when we create the state.
    rthread main;

  private:
    cthread L;
  } thread;

  /**************************************************************
  ** Strings
  ***************************************************************/
  struct String {
    String( cthread cth ) : L( cth ) {}

    rstring create( std::string_view sv ) noexcept;

  private:
    cthread L;
  } string;

  /**************************************************************
  ** Tables
  ***************************************************************/
  struct Table {
    Table( cthread cth );

    table global;
    table create() noexcept;

  private:
    cthread L;
  } table;

  /**************************************************************
  ** Indexer
  ***************************************************************/
  template<typename U>
  auto operator[]( U&& idx ) noexcept {
    return table.global[std::forward<U>( idx )];
  }

private:
  state( state const& ) = delete;
  state( state&& )      = delete;
  state& operator=( state const& ) = delete;
  state& operator=( state&& ) = delete;
};

// Cannot push an entire global state. You can push a thread
// though (rthread).
void push( cthread, state const& ) = delete;

} // namespace lua
