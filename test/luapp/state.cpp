/****************************************************************
**state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Unit tests for the src/luapp/state.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/state.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[lua-state] views don't free" ) {
  state view1 = state::view( L );
  state view2 = state::view( L );
  state view3 = state::view( L );
  state view4 = state::view( L );
  state view5 = state::view( L );
}

LUA_TEST_CASE( "[lua-state] standard tables" ) {
  C.openlibs();

  table G = st.table.global();
  push( L, G );
  C.getfield( -1, "tostring" );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.type_of( -1 ) == type::function );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  table empty1 = st.table.create();
  push( L, empty1 );
  push( L, empty1 );
  REQUIRE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  table empty2 = st.table.create();

  push( L, empty1 );
  push( L, empty2 );
  REQUIRE_FALSE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );
}

LUA_TEST_CASE( "[lua-state] string gen" ) {
  rstring s1 = st.string.create( "hello" );
  rstring s2 = st.string.create( "world" );
  rstring s3 = st.string.create( "hello" );

  REQUIRE( s1 == "hello" );
  REQUIRE( s2 == "world" );
  REQUIRE( s3 == "hello" );

  REQUIRE( s1 == s1 );
  REQUIRE( s2 == s2 );
  REQUIRE( s3 == s3 );

  REQUIRE( s1 != s2 );
  REQUIRE( s1 == s3 );
  REQUIRE( s2 != s3 );
}

LUA_TEST_CASE( "[lua-state] state indexing" ) {
  st["a"]             = st.table.create();
  st["a"][5]          = st.table.create();
  st["a"][5]["world"] = 9;

  table G = st.table.global();

  REQUIRE( ( G["a"][5]["world"] == 9 ) );
  REQUIRE( ( G["a"] == st["a"] ) );
  REQUIRE( ( G["a"][5] == st["a"][5] ) );
  REQUIRE( ( G["a"][5] != st["a"] ) );
}

LUA_TEST_CASE( "[lua-state] script loading" ) {
  rfunction f = st.script.load( R"(
    return 'hello'
  )" );

  REQUIRE( f() == "hello" );
}

LUA_TEST_CASE( "[lua-state] script run unsafe" ) {
  REQUIRE( st.script.run<string>( R"(
    return 'hello'
  )" ) == "hello" );
}

LUA_TEST_CASE( "[lua-state] script run unsafe void" ) {
  st.script( R"(
    res = 'hello'
  )" );
  REQUIRE( st["res"] == "hello" );
}

LUA_TEST_CASE( "[lua-state] script run safe" ) {
  C.openlibs();
  lua_valid   v = st.script.run_safe( R"(
    assert( 1 == 2 )
  )" );
  char const* err =
      "[string \"...\"]:2: assertion failed!\n"
      "stack traceback:\n"
      "\t[C]: in function 'assert'\n"
      "\t[string \"...\"]:2: in main chunk";

  REQUIRE( v == lua_invalid( err ) );
}

LUA_TEST_CASE( "[lua-state] thread create" ) {
  rthread th_main = st.thread.main();
  REQUIRE( th_main.is_main() );
  rthread th2 = st.thread.create();
  REQUIRE( !th2.is_main() );
  REQUIRE( th2 != th_main );
}

LUA_TEST_CASE( "[lua-state] thread create coro" ) {
  st.script.run( "function f() end" );
  rfunction f      = st["f"].as<rfunction>();
  rthread   coro   = st.thread.create_coro( f );
  cthread   L_coro = coro.cthread();
  c_api     C_coro( L_coro );
  REQUIRE( C_coro.stack_size() == 1 );
  REQUIRE( C_coro.type_of( -1 ) == type::function );
  rfunction f2( coro.cthread(), C_coro.ref_registry() );
  REQUIRE( f2 == f );
}

LUA_TEST_CASE( "[lua-state] cast" ) {
  SECTION( "int" ) {
    st["x"] = 5;
    REQUIRE( st.as<int>( st["x"] ) == 5 );
  }
  SECTION( "string" ) {
    st["x"] = "hello";
    REQUIRE( st.as<string>( st["x"] ) == "hello" );
    st["x"] = 5;
    REQUIRE( st.as<string>( st["x"] ) == "5" );
  }
  SECTION( "table" ) {
    st["x"] = st.table.create();
    REQUIRE( st.as<table>( st["x"] ) == st["x"] );
  }
}

LUA_TEST_CASE( "[lua-state] function" ) {
  rfunction f1 =
      st.function.create( []( int n ) { return n + 1; } );
  int       m = 5;
  rfunction f2 =
      st.function.create( [m]( int n ) { return n + m; } );

  REQUIRE( f1( 7 ) == 8 );
  REQUIRE( f2( 7 ) == 12 );
}

} // namespace
} // namespace lua
