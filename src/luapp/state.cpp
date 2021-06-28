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
#include "error.hpp"

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
  : Base( cth, /*own=*/false ),
    thread( resource() ),
    string( resource() ),
    table( resource() ),
    lib( resource() ),
    usertype( resource() ),
    script( resource() ) {}

state::state()
  : Base( luaL_newstate(), /*own=*/true ),
    thread( resource() ),
    string( resource() ),
    table( resource() ),
    lib( resource() ),
    usertype( resource() ),
    script( resource() ) {
  // This will be called whenever an error happens in a Lua call
  // that is not run in a protected environment. For example, if
  // we call lua_getglobal from C++ (outside of a pcall) and it
  // raises an error, this panic function will be called.
  lua_atpanic( resource(), panic );
}

void state::free_resource() { lua_close( resource() ); }

/****************************************************************
** Threads
*****************************************************************/
state::Thread::Thread( cthread cth ) : L( cth ) {}

rthread state::Thread::main() noexcept {
  c_api C( L );
  C.pushthread();
  return rthread( L, C.ref_registry() );
}

rthread state::Thread::create() noexcept {
  c_api C( L );
  (void)C.newthread();
  return rthread( L, C.ref_registry() );
}

/****************************************************************
** Tables
*****************************************************************/
state::Table::Table( cthread cth ) : L( cth ) {}

table state::Table::global() noexcept {
  c_api C( L );
  C.pushglobaltable();
  return lua::table( L, C.ref_registry() );
}

table state::Table::create() noexcept {
  c_api C( L );
  C.newtable();
  return lua::table( L, C.ref_registry() );
}

/****************************************************************
** Libs
*****************************************************************/
void state::Lib::open_all() {
  c_api C( L );
  C.openlibs();
}

/****************************************************************
** Usertype
*****************************************************************/

/****************************************************************
** Strings
*****************************************************************/
rstring state::String::create( string_view sv ) noexcept {
  c_api C( L );
  C.push( sv );
  return rstring( L, C.ref_registry() );
}

/****************************************************************
** Scripts
*****************************************************************/
state::Script::Script( cthread cth ) : L( cth ) {}

lua_expect<rfunction> state::Script::load_safe(
    std::string_view code ) noexcept {
  c_api C( L );
  HAS_VALUE_OR_RET(
      C.loadstring( std::string( code ).c_str() ) );
  return rfunction( L, C.ref_registry() );
}

rfunction state::Script::load( string_view code ) noexcept {
  lua_expect<rfunction> res = load_safe( code );
  if( !res.has_value() )
    throw_lua_error( L, "failed to load code string: {}", res );
  return *res;
}

void state::Script::operator()( string_view code ) {
  lua::push( L, load( code ) );
  return call_lua_unsafe_and_get<void>( L );
}

lua_valid state::Script::load_file_safe(
    std::string_view file ) {
  c_api C( L );
  return C.loadfile( std::string( file ).c_str() );
}

void state::Script::load_file( std::string_view file ) {
  c_api     C( L );
  lua_valid res = load_file_safe( file );
  if( !res ) throw_lua_error( L, "{}", res );
}

} // namespace lua
