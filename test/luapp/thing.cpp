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
  REQUIRE( th.type() == e_lua_type::light_userdata );
  REQUIRE( th.index() == 2 );

  int x = 3;
  th    = (void*)&x;
  REQUIRE( th );
  REQUIRE( th != nullptr );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::light_userdata );
  REQUIRE( th.index() == 2 );
  REQUIRE( th == &x );
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
  REQUIRE( th.type() == e_lua_type::light_userdata );
  REQUIRE( th == nullptr );
  REQUIRE( fmt::format( "{}", th ) == "<lightuserdata:0x0>" );
  int x = 3;
  p     = &x;
  th    = p;
  REQUIRE( th.type() == e_lua_type::light_userdata );
  REQUIRE( th == &x );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "<lightuserdata:0x[0-9a-z]+>" ) );
}

} // namespace
} // namespace luapp
