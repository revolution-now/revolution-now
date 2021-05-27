/****************************************************************
**c-api.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-27.
*
* Description: Simple wrapper for Lua C API calls.
*
*****************************************************************/
#include "c-api.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lauxlib.h"
#include "lualib.h"

// C++ standard library
#include <array>

using namespace std;

namespace luapp {

/****************************************************************
** c_api
*****************************************************************/
c_api::c_api() {
  lua_State* st = luaL_newstate();
  CHECK( st != nullptr );
  L = st;
}

c_api::~c_api() noexcept { lua_close( L ); }

/**************************************************************
** Lua C Function Wrappers.
***************************************************************/
void c_api::openlibs() { luaL_openlibs( L ); }

lua_valid c_api::dofile( char const* file ) {
  // luaL_dofile: [-0, +?, e]
  int error = luaL_dofile( L, file );
  if( error ) {
    string msg = lua_tostring( L, -1 );
    lua_pop( L, 1 );
    return msg;
  }
  return base::valid;
}

lua_valid c_api::dofile( string const& file ) {
  return dofile( file.c_str() );
}

int c_api::gettop() const { return lua_gettop( L ); }
int c_api::stack_size() const { return gettop(); }

lua_valid c_api::setglobal( char const* key ) {
  enforce_stack_size_ge( 1 );
  // Apparently the docs say that this can raise an error
  // ([-1,+0,e]) but not sure what that means (exception?).
  ::lua_setglobal( L, key );
  return base::valid;
}

lua_valid c_api::setglobal( string const& key ) {
  return setglobal( key.c_str() );
}

lua_expect<e_lua_type> c_api::getglobal( char const* name ) {
  int type = lua_getglobal( L, name );
  enforce_stack_size_ge( 1 );
  if( type_of( -1 ) == e_lua_type::nil ) {
    pop(); // pop nil off the stack.
    return fmt::format( "global `{}` not found.", name );
  }
  return lua_type_to_enum( type );
}

lua_expect<e_lua_type> c_api::getglobal(
    std::string const& name ) {
  return getglobal( name.c_str() );
}

lua_valid c_api::loadstring( char const* script ) {
  lua_valid res = base::valid;
  // [-0, +1, –]
  if( luaL_loadstring( L, script ) == LUA_OK ) {
    // Pushes a function onto the stack.
    enforce_stack_size_ge( 1 );
  } else {
    res = lua_tostring( L, -1 );
    lua_pop( L, 1 );
  }
  return res;
}

lua_valid c_api::loadstring( string const& script ) {
  return loadstring( script.c_str() );
}

lua_valid c_api::pcall( pcall_options const& o ) {
  CHECK( o.nargs >= 0 );
  CHECK( o.nresults >= 0 );
  // Function object plus args should be on the stack at least.
  enforce_stack_size_ge( o.nargs + 1 );
  lua_valid res = base::valid;
  // No matter what happens, lua_pcall will remove the function
  // and arguments from the stack.
  int err = lua_pcall( L, o.nargs, o.nresults, /*msgh=*/0 );
  if( err == LUA_OK ) {
    enforce_stack_size_ge( o.nresults );
  } else {
    // lua_pcall will have pushed a single value onto the stack,
    // which will be the error object.
    res = lua_tostring( L, -1 );
    lua_pop( L, 1 );
  }
  return res;
}

void c_api::pop( int n ) {
  CHECK_GE( stack_size(), n );
  lua_pop( L, n );
}

e_lua_type c_api::lua_type_to_enum( int type ) const {
  CHECK( type != LUA_TNONE, "type ({}) not valid.", type );
  CHECK( type >= 0 );
  CHECK( type < kNumLuaTypes,
         "a new lua type may have been added." );
  return static_cast<e_lua_type>( type );
}

// The Lua types are defined in lua.h, as of Lua 5.3:
//
//   LUA_TNIL		        0
//   LUA_TBOOLEAN		    1
//   LUA_TLIGHTUSERDATA	2
//   LUA_TNUMBER		    3
//   LUA_TSTRING		    4
//   LUA_TTABLE		      5
//   LUA_TFUNCTION		  6
//   LUA_TUSERDATA		  7
//   LUA_TTHREAD		    8
//
e_lua_type c_api::type_of( int idx ) {
  int res = lua_type( L, idx );
  CHECK( res != LUA_TNONE, "index ({}) not valid.", idx );
  return lua_type_to_enum( res );
}

char const* c_api::type_name( e_lua_type type ) {
  return ::lua_typename( L, static_cast<int>( type ) );
}

/**************************************************************
** Error checking helpers.
***************************************************************/
void c_api::enforce_stack_size_ge( int s ) {
  CHECK( s >= 0 );
  if( stack_size() >= s ) return;
  FATAL(
      "stack size expected to have size at least {} but "
      "instead found {}.",
      s, stack_size() );
}

lua_valid c_api::enforce_type_of( int idx, e_lua_type type ) {
  if( type_of( idx ) == type ) return base::valid;
  return "type of element at index " + to_string( idx ) +
         " expected to be " + string( type_name( type ) ) +
         ", but instead is " +
         string( type_name( type_of( idx ) ) );
}

/****************************************************************
** to_str
*****************************************************************/
void to_str( luapp::e_lua_type t, string& out ) {
  using namespace luapp;
  string_view s = "unknown";
  switch( t ) {
    case e_lua_type::nil: s = "nil"; break;
    case e_lua_type::boolean: s = "boolean"; break;
    case e_lua_type::light_userdata: s = "light_userdata"; break;
    case e_lua_type::number: s = "number"; break;
    case e_lua_type::string: s = "string"; break;
    case e_lua_type::table: s = "table"; break;
    case e_lua_type::function: s = "function"; break;
    case e_lua_type::userdata: s = "userdata"; break;
    case e_lua_type::thread: s = "thread"; break;
  }
  CHECK( s != "unknown" );
  out += string( s );
}

} // namespace luapp
