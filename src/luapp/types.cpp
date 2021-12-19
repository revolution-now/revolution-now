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
#include "scratch.hpp"
#include "state.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lua.h"

// C++ standard library
#include <string_view>

using namespace std;

namespace lua {

/****************************************************************
** Lua types
*****************************************************************/
#define ASSERT_MATCH( e, lua_e ) \
  static_assert( static_cast<int>( type::e ) == lua_e )

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

// TODO: add test for this
char const* type_name( cthread L, int idx ) noexcept {
  return lua_typename( L, lua_type( L, idx ) );
}

type type_of_idx( cthread L, int idx ) noexcept {
  return c_api( L ).type_of( idx );
}

void pop_stack( cthread L, int n ) noexcept {
  c_api( L ).pop( n );
}

bool operator==( lightuserdata const& l,
                 lightuserdata const& r ) {
  return l.get() == r.get();
}

bool operator!=( lightuserdata const& l,
                 lightuserdata const& r ) {
  return l.get() != r.get();
}

/****************************************************************
** equality
*****************************************************************/
namespace {

template<typename Left, typename Right>
bool eq_value_and_value( Left const& l, Right const& r ) {
  c_api C( scratch_state().thread.main().cthread() );
  C.push( l );
  C.push( r );
  bool res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

} // namespace

#define EQ_VAL_VAL_IMPL( left_t, right_t )               \
  bool operator==( left_t const& l, right_t const& r ) { \
    return eq_value_and_value( l, r );                   \
  }

EQ_VAL_VAL_IMPL( nil_t, boolean );
EQ_VAL_VAL_IMPL( nil_t, lightuserdata );
EQ_VAL_VAL_IMPL( nil_t, integer );
EQ_VAL_VAL_IMPL( nil_t, floating );
EQ_VAL_VAL_IMPL( boolean, lightuserdata );
EQ_VAL_VAL_IMPL( boolean, integer );
EQ_VAL_VAL_IMPL( boolean, floating );
EQ_VAL_VAL_IMPL( lightuserdata, integer );
EQ_VAL_VAL_IMPL( lightuserdata, floating );
EQ_VAL_VAL_IMPL( integer, floating );

/******************************************************************
** push
*******************************************************************/
void lua_push( cthread L, nil_t ) {
  c_api C( L );
  C.push( nil );
}

void lua_push( cthread L, boolean b ) {
  c_api C( L );
  C.push( b );
}

void lua_push( cthread L, integer i ) {
  c_api C( L );
  C.push( i );
}

void lua_push( cthread L, floating f ) {
  c_api C( L );
  C.push( f );
}

void lua_push( cthread L, lightuserdata lud ) {
  c_api C( L );
  C.push( lud );
}

void lua_push( cthread L, string_view sv ) {
  c_api C( L );
  C.push( sv );
}

/******************************************************************
** lua_get
*******************************************************************/
base::maybe<boolean> lua_get( cthread L, int idx,
                              tag<boolean> ) {
  return c_api( L ).get<boolean>( idx );
}

base::maybe<integer> lua_get( cthread L, int idx,
                              tag<integer> ) {
  return c_api( L ).get<integer>( idx );
}

base::maybe<floating> lua_get( cthread L, int idx,
                               tag<floating> ) {
  return c_api( L ).get<floating>( idx );
}

base::maybe<lightuserdata> lua_get( cthread L, int idx,
                                    tag<lightuserdata> ) {
  return c_api( L ).get<lightuserdata>( idx );
}

base::maybe<string> lua_get( cthread L, int idx, tag<string> ) {
  return c_api( L ).get<string>( idx );
}

base::maybe<bool> lua_get( cthread L, int idx, tag<bool> ) {
  return c_api( L ).get<bool>( idx );
}

base::maybe<int> lua_get( cthread L, int idx, tag<int> ) {
  return c_api( L ).get<int>( idx );
}

base::maybe<double> lua_get( cthread L, int idx, tag<double> ) {
  return c_api( L ).get<double>( idx );
}

base::maybe<void*> lua_get( cthread L, int idx, tag<void*> ) {
  return c_api( L ).get<void*>( idx );
}

/******************************************************************
** to_str
*******************************************************************/
void to_str( nil_t, std::string& out, base::ADL_t ) {
  out += string_view( "nil" );
}

void to_str( lightuserdata const& o, std::string& out,
             base::ADL_t ) {
  out += fmt::format( "<lightuserdata:{}>", o.get() );
}

#define TYPE_CASE( e ) \
  case type::e: s = #e; break

void to_str( type t, string& out, base::ADL_t ) {
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

/****************************************************************
** helpers
*****************************************************************/
int upvalue_index( int upvalue ) {
  return lua_upvalueindex( upvalue );
}

} // namespace lua
