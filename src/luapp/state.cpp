/****************************************************************
**state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII type for global lua state.
*
*****************************************************************/
#include "state.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"

// C++ standard library
#include <string>

using namespace std;

namespace lua {

namespace {

[[noreturn]] int panic( lua_State* L ) {
  string err = lua_tostring( L, -1 );
  FATAL( "uncaught lua error: {}", err );
}

} // namespace

state::state() : L( luaL_newstate() ) {
  // This will be called whenever an error happens in a Lua call
  // that is not run in a protected environment. For example, if
  // we call lua_getglobal from C++ (outside of a pcall) and it
  // raises an error, this panic function will be called.
  lua_atpanic( L, panic );
}

state::~state() noexcept { close(); }

void state::close() {
  if( L != nullptr ) {
    lua_close( L );
    L = nullptr;
  }
}

} // namespace lua
