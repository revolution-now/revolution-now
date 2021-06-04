/****************************************************************
**state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: Unit tests for the src/luapp/state.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/state.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::luapp::e_lua_type );

namespace luapp {
namespace {

using namespace std;

TEST_CASE( "[state] creation/destruction" ) { state st; }

TEST_CASE( "[state] tables" ) {
  state  st;
  c_api& C = st.api();
  REQUIRE( C.getglobal( "t1" ) == e_lua_type::nil );
  C.pop();
  REQUIRE( C.stack_size() == 0 );

  SECTION( "empty" ) { st.tables( "" ); }

  SECTION( "single" ) {
    st.tables( "t1" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 1 );
    C.pop();
  }

  SECTION( "double" ) {
    st.tables( "t1.t2" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 2 );
    C.pop( 2 );
  }

  SECTION( "triple" ) {
    st.tables( "t1.t2.t3" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "tables already present" ) {
    st.tables( "t1" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::nil );
    REQUIRE( C.stack_size() == 2 );
    C.pop( 2 );
    st.tables( "t1.t2" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::nil );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
    st.tables( "t1.t2.t3" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
    st.tables( "t1.t2.t3" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t2" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "triple 2" ) {
    st.tables( "hello_world.yes123x._" );
    REQUIRE( C.getglobal( "hello_world" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "yes123x" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "_" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "spaces" ) {
    st.tables( " t1. t2" );
    REQUIRE( C.getglobal( " t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, " t2" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 2 );
    C.pop( 2 );
  }

  SECTION( "with reserved" ) {
    st.tables( "t1.if.t3" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "if" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  SECTION( "bad identifier" ) {
    st.tables( "t1.x-z.t3" );
    REQUIRE( C.getglobal( "t1" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "x-z" ) == e_lua_type::table );
    REQUIRE( C.getfield( -1, "t3" ) == e_lua_type::table );
    REQUIRE( C.stack_size() == 3 );
    C.pop( 3 );
  }

  REQUIRE( C.stack_size() == 0 );
}

} // namespace
} // namespace luapp
