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

// luapp
#include "c-api.hpp"

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

state::state( cthread cth )
  : L( cth ),
    own_( false ),
    thread( L ),
    string( L ),
    table( L ) {
  // This will be called whenever an error happens in a Lua call
  // that is not run in a protected environment. For example, if
  // we call lua_getglobal from C++ (outside of a pcall) and it
  // raises an error, this panic function will be called.
  lua_atpanic( L, panic );
}

state::state() : state( luaL_newstate() ) { own_ = true; }

state::~state() noexcept { close(); }

void state::close() {
  if( !own_ ) return;
  lua_close( L );
  L    = nullptr;
  own_ = false;
}

/****************************************************************
** Threads
*****************************************************************/
state::Thread::Thread( cthread cth ) : L( cth ) {}

rthread state::Thread::main() const noexcept {
  c_api C( L );
  C.pushthread();
  return rthread( L, C.ref_registry() );
}

/****************************************************************
** Tables
*****************************************************************/
state::Table::Table( cthread cth ) : L( cth ) {}

table state::Table::global() const noexcept {
  c_api C( L );
  C.pushglobaltable();
  return lua::table( L, C.ref_registry() );
}

table state::Table::create() const noexcept {
  c_api C( L );
  C.newtable();
  return lua::table( L, C.ref_registry() );
}

/****************************************************************
** Strings
*****************************************************************/
rstring state::String::create( string_view sv ) const noexcept {
  c_api C( L );
  C.push( sv );
  return rstring( L, C.ref_registry() );
}

} // namespace lua
