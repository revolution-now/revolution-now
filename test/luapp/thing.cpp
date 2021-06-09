/****************************************************************
**thing.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: Unit tests for the src/luapp/thing.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/thing.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::luapp::e_lua_type );
FMT_TO_CATCH( ::luapp::thing );

namespace luapp {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matches;

TEST_CASE( "[thing] static checks" ) {
  static_assert( is_default_constructible_v<thing> );

  static_assert( is_nothrow_move_constructible_v<thing> );
  static_assert( is_nothrow_move_assignable_v<thing> );
  static_assert( !is_copy_constructible_v<thing> );
  static_assert( !is_copy_assignable_v<thing> );

  // This is to prevent assigning strings or string literals. We
  // can't use them to create Lua strings since we may not have
  // an L, and we probably don't want to create light userdatas
  // from them, so just better to prevent it.
  static_assert( !is_assignable_v<thing, char const( & )[6]> );
  static_assert( !is_assignable_v<thing, char const( * )[6]> );
  static_assert( !is_assignable_v<thing, char const*> );
  static_assert( !is_assignable_v<thing, char( & )[6]> );
  static_assert( !is_assignable_v<thing, char( * )[6]> );
  static_assert( !is_assignable_v<thing, char*> );
  static_assert( !is_assignable_v<thing, std::string> );
  static_assert( !is_assignable_v<thing, std::string_view> );

  // This is kind of weird, so disallow it.
  static_assert( !is_assignable_v<thing, void const*> );

  // Make sure this is allowed as it is for light userdata.
  static_assert( is_assignable_v<thing, void*> );

  static_assert( is_nothrow_convertible_v<thing, bool> );
}

TEST_CASE( "[thing] defaults to nil + equality with nil" ) {
  thing th;
  REQUIRE( th == nil );
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( !th );
}

TEST_CASE( "[thing] nil assignment / equality" ) {
  thing th = nil;
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( th.index() == 0 );
  REQUIRE( th == nil );
  REQUIRE( !th );
  th = nil;
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( th.index() == 0 );
  REQUIRE( th == nil );
  REQUIRE( !th );
}

TEST_CASE( "[thing] bool assignment / equality" ) {
  thing th = true;
  REQUIRE( th );
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th.index() == 1 );
  REQUIRE( th == true );
  th = false;
  REQUIRE( th != true );
  REQUIRE( th == false );
  REQUIRE( th.index() == 1 );
  REQUIRE( !th );
}

TEST_CASE( "[thing] int assignment / equality" ) {
  thing th = 5;
  REQUIRE( th );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
  REQUIRE( th == 5 );
  th = 6;
  REQUIRE( th );
  REQUIRE( th != 5 );
  REQUIRE( th == 6 );
  th = 7L;
  REQUIRE( th == 7L );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
  th = -1;
  REQUIRE( th );
  REQUIRE( th == -1 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
  th = 0;
  REQUIRE( th ); // Lua's rules.
  REQUIRE( th == 0 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
}

TEST_CASE( "[thing] double assignment / equality" ) {
  thing th = 5.5;
  REQUIRE( th );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 4 );
  REQUIRE( th == 5.5 );
  th = 6.5;
  REQUIRE( th );
  REQUIRE( th != 5.5 );
  REQUIRE( th == 6.5 );
  th = -1.3;
  REQUIRE( th );
  REQUIRE( th == -1.3 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 4 );
  th = 0.0;
  REQUIRE( th ); // Lua's rules.
  REQUIRE( th == 0.0 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 4 );
}

TEST_CASE( "[thing] lightuserdata assignment / equality" ) {
  thing th = nullptr;
  REQUIRE( th ); // Lua's rules.
  REQUIRE( th == nullptr );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th.index() == 2 );

  int x = 3;
  th    = (void*)&x;
  REQUIRE( th );
  REQUIRE( th != nullptr );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th.index() == 2 );
  REQUIRE( th == (void*)&x );
}

TEST_CASE( "[thing] thing inequality" ) {
  vector<thing> v;
  v.push_back( 5 );
  v.push_back( 5.5 );
  v.push_back( true );
  v.push_back( (void*)"hello" );
  v.push_back( nil );

  for( int i = 0; i < int( v.size() ); ++i ) {
    for( int j = 0; j < int( v.size() ); ++j ) {
      if( i == j ) {
        REQUIRE( v[i] == v[j] );
        REQUIRE( v[j] == v[i] );
      } else {
        REQUIRE( v[i] != v[j] );
        REQUIRE( v[j] != v[i] );
      }
    }
  }
}

TEST_CASE( "[thing] thing inequality convertiblae" ) {
  thing th1 = 5;
  thing th2 = 5.0;
  thing th3 = true;

  REQUIRE( th1 == th2 );
  REQUIRE( th2 == th1 );
  REQUIRE( th3 != th1 );
  REQUIRE( th3 != th2 );
  REQUIRE( th1 != th3 );
  REQUIRE( th2 != th3 );
}

TEST_CASE( "[thing] fmt/to_str" ) {
  thing th;
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( fmt::format( "{}", th ) == "nil" );

  th = true;
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th == true );
  REQUIRE( fmt::format( "{}", th ) == "true" );

  th = false;
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th == false );
  REQUIRE( fmt::format( "{}", th ) == "false" );

  th = 42;
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th == 42 );
  REQUIRE( fmt::format( "{}", th ) == "42" );

  th = 3.5;
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th == 3.5 );
  REQUIRE( fmt::format( "{}", th ) == "3.5" );

  void* p = nullptr;
  th      = p;
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th == nullptr );
  REQUIRE( fmt::format( "{}", th ) == "<lightuserdata:0x0>" );
  int x = 3;
  p     = &x;
  th    = p;
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th == (void*)&x );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "<lightuserdata:0x[0-9a-z]+>" ) );
}

} // namespace
} // namespace luapp
