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

state::state()
  : L( luaL_newstate() ), thread( L ), string( L ), table( L ) {
  // This will be called whenever an error happens in a Lua call
  // that is not run in a protected environment. For example, if
  // we call lua_getglobal from C++ (outside of a pcall) and it
  // raises an error, this panic function will be called.
  lua_atpanic( L, panic );
}

state::~state() noexcept { close(); }

void state::close() {
  if( !alive() ) return;

  // These need to be released before we destroy the Lua state,
  // otherwise their destructors will try to use it to release
  // themselves.
  thread.main.release();
  table.global.release();

  lua_close( L );
  L = nullptr;
}

/****************************************************************
** Threads
*****************************************************************/
state::Thread::Thread( cthread cth )
  : main( cth, ( c_api( cth ).pushthread(),
                 c_api( cth ).ref_registry() ) ),
    L( cth ) {
  (void)L;
}

/****************************************************************
** Tables
*****************************************************************/
state::Table::Table( cthread cth )
  : global( cth, ( c_api( cth ).pushglobaltable(),
                   c_api( cth ).ref_registry() ) ),
    L( cth ) {}

table state::Table::create() noexcept {
  c_api C( L );
  C.newtable();
  return lua::table( C.this_cthread(), C.ref_registry() );
}

/****************************************************************
** Strings
*****************************************************************/
rstring state::String::create( string_view sv ) noexcept {
  c_api C( L );
  C.push( sv );
  return rstring( C.this_cthread(), C.ref_registry() );
}

} // namespace lua
