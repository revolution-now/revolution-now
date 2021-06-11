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

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lua.h"

// C++ standard library
#include <string_view>

using namespace std;

namespace lua {

#define ASSERT_MATCH( e, lua_e ) \
  static_assert( static_cast<int>( e_lua_type::e ) == lua_e )

// clang-format off
ASSERT_MATCH( nil,            LUA_TNIL           );
ASSERT_MATCH( boolean,        LUA_TBOOLEAN       );
ASSERT_MATCH( lightuserdata,  LUA_TLIGHTUSERDATA );
ASSERT_MATCH( number,         LUA_TNUMBER        );
ASSERT_MATCH( string,         LUA_TSTRING        );
ASSERT_MATCH( table,          LUA_TTABLE         );
ASSERT_MATCH( function,       LUA_TFUNCTION      );
ASSERT_MATCH( userdata,       LUA_TUSERDATA      );
ASSERT_MATCH( thread,         LUA_TTHREAD        );
// clang-format on

/******************************************************************
** push
*******************************************************************/
void push( cthread L, nil_t ) {
  c_api C( L );
  C.push( nil );
}

void push( cthread L, boolean b ) {
  c_api C( L );
  C.push( b );
}

void push( cthread L, bool b ) {
  c_api C( L );
  C.push( b );
}

void push( cthread L, integer i ) {
  c_api C( L );
  C.push( i );
}

void push( cthread L, int i ) {
  c_api C( L );
  C.push( i );
}

void push( cthread L, floating f ) {
  c_api C( L );
  C.push( f );
}

void push( cthread L, double f ) {
  c_api C( L );
  C.push( f );
}

void push( cthread L, lightuserdata lud ) {
  c_api C( L );
  C.push( lud );
}

void push( cthread L, void* lud ) {
  c_api C( L );
  C.push( lud );
}

void push( cthread L, string_view sv ) {
  c_api C( L );
  C.push( sv );
}

void push( cthread L, char const* p ) {
  c_api C( L );
  C.push( string_view( p ) );
}

/******************************************************************
** to_str
*******************************************************************/
void to_str( nil_t, std::string& out ) {
  out += string_view( "nil" );
}

void to_str( lightuserdata const& o, std::string& out ) {
  out += fmt::format( "<lightuserdata:{}>", o.get() );
}

#define TYPE_CASE( e ) \
  case e_lua_type::e: s = #e; break

void to_str( e_lua_type t, string& out ) {
  string_view s = "unknown";
  switch( t ) {
    TYPE_CASE( nil );
    TYPE_CASE( boolean );
    TYPE_CASE( lightuserdata );
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

} // namespace lua
