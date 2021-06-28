/****************************************************************
**rthread.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              threads.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** rthread
*****************************************************************/
struct rthread : public any {
  using Base = any;

  rthread( lua::cthread L, int ref );

  friend base::maybe<rthread> lua_get( lua::cthread L, int idx,
                                       tag<rthread> );

  // In an rthread, the particular `L' held represents the
  // thread. This is unlike other objects where the L is only
  // held for access to the global state.
  lua::cthread cthread() const noexcept { return L; }

  bool is_main() const noexcept;
};

static_assert( Stackable<rthread> );

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::rthread );
