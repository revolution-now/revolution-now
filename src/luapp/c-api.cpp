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
#include <cmath>

using namespace std;

namespace luapp {

using ::base::maybe;
using ::base::nothing;

/****************************************************************
** errors
*****************************************************************/
lua_valid lua_invalid( lua_error_t err ) {
  return base::invalid<lua_error_t>( move( err ) );
}

/****************************************************************
** c_api
*****************************************************************/
c_api::c_api() {
  L = ::luaL_newstate();
  CHECK( L != nullptr );
}

c_api::~c_api() noexcept {
  // Not sure if this check is a good idea...
  DCHECK( stack_size() == 0 );
  ::lua_close( L );
}

/****************************************************************
** Lua C Function Wrappers.
*****************************************************************/
void c_api::openlibs() { luaL_openlibs( L ); }

lua_valid c_api::dofile( char const* file ) {
  // luaL_dofile: [-0, +?, e]
  if( luaL_dofile( L, file ) ) return pop_and_return_error();
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

lua_expect<e_lua_type> c_api::getglobal( string const& name ) {
  return getglobal( name.c_str() );
}

lua_valid c_api::loadstring( char const* script ) {
  lua_valid res = base::valid;
  // [-0, +1, –]
  if( luaL_loadstring( L, script ) == LUA_OK )
    // Pushes a function onto the stack.
    enforce_stack_size_ge( 1 );
  else
    return pop_and_return_error();
  return res;
}

lua_valid c_api::loadstring( string const& script ) {
  return loadstring( script.c_str() );
}

lua_valid c_api::dostring( char const* script ) {
  HAS_VALUE_OR_RET( loadstring( script ) );
  enforce_stack_size_ge( 1 );
  return pcall( /*nargs=*/0, /*nresults=*/LUA_MULTRET );
}

lua_valid c_api::dostring( string const& script ) {
  return dostring( script.c_str() );
}

lua_valid c_api::pcall( int nargs, int nresults ) {
  CHECK( nargs >= 0 );
  CHECK( nresults >= 0 || nresults == LUA_MULTRET );
  // Function object plus args should be on the stack at least.
  enforce_stack_size_ge( nargs + 1 );
  lua_valid res = base::valid;
  // No matter what happens, lua_pcall will remove the function
  // and arguments from the stack.
  int err = ::lua_pcall( L, nargs, nresults, /*msgh=*/0 );
  if( err == LUA_OK ) {
    if( nresults != LUA_MULTRET )
      enforce_stack_size_ge( nresults );
  } else {
    // lua_pcall will have pushed a single value onto the stack,
    // which will be the error object.
    return pop_and_return_error();
  }
  return res;
}

void c_api::push( nil_t ) { ::lua_pushnil( L ); }

void c_api::push( base::safe::boolean b ) {
  int b_int = b; // just to make it explicit.
  ::lua_pushboolean( L, b_int );
}

void c_api::push( base::safe::integral<lua_Integer> n ) {
  ::lua_pushinteger( L, n );
}

void c_api::push( base::safe::floating<lua_Number> d ) {
  ::lua_pushnumber( L, d );
}

void c_api::push( string_view sv ) {
  CHECK( sv.data() != nullptr );
  // Pushes the string pointed to by s with size len onto the
  // stack. Lua makes (or reuses) an internal copy of the given
  // string, so the memory at s can be freed or reused immedi-
  // ately after the function returns. The string can contain any
  // binary data, including embedded zeros.
  //
  // Returns a pointer to the internal copy of the string.
  // [-0, +1, m]
  ::lua_pushlstring( L, sv.data(), sv.size() );
}

void c_api::pop( int n ) {
  CHECK_GE( stack_size(), n );
  lua_pop( L, n );
}

bool c_api::get( int idx, bool* ) const {
  validate_index( idx );
  // Converts the Lua value at the given index to a C boolean
  // value (0 or 1). Like all tests in Lua, lua_toboolean returns
  // true for any Lua value different from false and nil; other-
  // wise it returns false. (If you want to accept only actual
  // boolean values, use lua_isboolean to test the value's type.)
  // [-0, +0, –]
  int i = ::lua_toboolean( L, idx );
  CHECK( i == 0 || i == 1 );
  return bool( i );
}

maybe<lua_Integer> c_api::get( int idx, lua_Integer* ) const {
  validate_index( idx );
  int is_num = 0;
  // Converts the Lua value at the given index to the signed in-
  // tegral type lua_Integer. The Lua value must be an integer,
  // or a number or string convertible to an integer (see
  // §3.4.3); otherwise, lua_tointegerx returns 0. If isnum is
  // not NULL, its referent is assigned a boolean value that in-
  // dicates whether the operation succeeded.
  // [-0, +0, –]
  lua_Integer i = ::lua_tointegerx( L, idx, &is_num );
  if( is_num != 0 ) return i;
  return nothing;
}

maybe<lua_Number> c_api::get( int idx, lua_Number* ) const {
  validate_index( idx );
  int is_num = 0;
  // lua_tonumberx: [-0, +0, –]
  //
  // Converts the Lua value at the given index to the C type
  // lua_Number (see lua_Number). The Lua value must be a number
  // or a string convertible to a number (see §3.4.3); otherwise,
  // lua_tonumberx returns 0.
  //
  // If isnum is not NULL, its referent is assigned a boolean
  // value that indicates whether the operation succeeded.
  lua_Number num = ::lua_tonumberx( L, idx, &is_num );
  if( is_num != 0 ) return num;
  return nothing;
}

maybe<string> c_api::get( int idx, string* ) const {
  validate_index( idx );
  // lua_tolstring:  [-0, +0, m]
  //
  // Converts the Lua value at the given index to a C string.If
  // len is not NULL, it sets *len with the string length.The Lua
  // value must be a string or a number; otherwise, the function
  // returns NULL.If the value is a number, then lua_tolstring
  // also changes the actual value in the stack to a string
  // .(This change confuses lua_next when lua_tolstring is ap-
  // plied to keys during a table traversal.)
  //
  // lua_tolstring returns a pointer to a string inside the Lua
  // state.This string always has a zero( '\0' ) after its last
  // character( as in C ), but can contain other zeros in its
  // body.
  //
  // Because Lua has garbage collection, there is no guarantee
  // that the pointer returned by lua_tolstring will be valid
  // after the corresponding Lua value is removed from the stack.
  size_t      len = 0;
  char const* p   = ::lua_tolstring( L, idx, &len );
  if( p == nullptr ) return nothing;
  DCHECK( int( len ) >= 0 );
  // Use the (pointer, size) constructor because we need to
  // specify the length, 1) so that std::string can pre-allocate,
  // and 2) because there may be zeroes inside the string before
  // the final null terminater.
  return string( p, len );
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
e_lua_type c_api::type_of( int idx ) const {
  validate_index( idx );
  int res = ::lua_type( L, idx );
  CHECK( res != LUA_TNONE, "index ({}) not valid.", idx );
  return lua_type_to_enum( res );
}

char const* c_api::type_name( e_lua_type type ) const {
  return ::lua_typename( L, static_cast<int>( type ) );
}

/****************************************************************
** Error checking helpers.
*****************************************************************/
void c_api::enforce_stack_size_ge( int s ) const {
  CHECK( s >= 0 );
  if( stack_size() >= s ) return;
  FATAL(
      "stack size expected to have size at least {} but "
      "instead found {}.",
      s, stack_size() );
}

lua_valid c_api::enforce_type_of( int        idx,
                                  e_lua_type type ) const {
  validate_index( idx );
  if( type_of( idx ) == type ) return base::valid;
  return "type of element at index " + to_string( idx ) +
         " expected to be " + string( type_name( type ) ) +
         ", but instead is " +
         string( type_name( type_of( idx ) ) );
}

lua_error_t c_api::pop_and_return_error() {
  enforce_stack_size_ge( 1 );
  lua_error_t res( lua_tostring( L, -1 ) );
  pop();
  return res;
}

void c_api::validate_index( int idx ) const {
  enforce_stack_size_ge( abs( idx ) );
}

/******************************************************************
** to_str
*******************************************************************/
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
