/****************************************************************
**types.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-30.
*
* Description: Types common to all luapp modules.
*
*****************************************************************/
#include "types.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lua.h"

using namespace std;

namespace luapp {

#define ASSERT_MATCH( e, lua_e ) \
  static_assert( static_cast<int>( e_lua_type::e ) == lua_e )

// clang-format off
ASSERT_MATCH( nil,            LUA_TNIL           );
ASSERT_MATCH( boolean,        LUA_TBOOLEAN       );
ASSERT_MATCH( light_userdata, LUA_TLIGHTUSERDATA );
ASSERT_MATCH( number,         LUA_TNUMBER        );
ASSERT_MATCH( string,         LUA_TSTRING        );
ASSERT_MATCH( table,          LUA_TTABLE         );
ASSERT_MATCH( function,       LUA_TFUNCTION      );
ASSERT_MATCH( userdata,       LUA_TUSERDATA      );
ASSERT_MATCH( thread,         LUA_TTHREAD        );
// clang-format on

namespace {
//
} // namespace

/******************************************************************
** to_str
*******************************************************************/
#define TYPE_CASE( e ) \
  case e_lua_type::e: s = #e; break

void to_str( luapp::e_lua_type t, string& out ) {
  using namespace luapp;
  string_view s = "unknown";
  switch( t ) {
    TYPE_CASE( nil );
    TYPE_CASE( boolean );
    TYPE_CASE( light_userdata );
    TYPE_CASE( number );
    TYPE_CASE( string );
    TYPE_CASE( table );
    TYPE_CASE( function );
    TYPE_CASE( userdata );
    TYPE_CASE( thread );
  }
  CHECK( s != "unknown" );
  out += string( s );
}

} // namespace luapp
