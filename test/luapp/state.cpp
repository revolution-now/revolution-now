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

FMT_TO_CATCH( ::lua::e_lua_type );

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[lua-state] standard tables" ) {
  C.openlibs();
  cthread L = C.this_cthread();

  table G = st.global_table();
  push( L, G );
  C.getfield( -1, "tostring" );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::function );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  table empty1 = st.new_table();
  push( L, empty1 );
  push( L, empty1 );
  REQUIRE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  table empty2 = st.new_table();
  push( L, empty1 );
  push( L, empty2 );
  REQUIRE_FALSE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );

  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-state] string gen" ) {
  rstring s1 = st.str( "hello" );
  rstring s2 = st.str( "world" );
  rstring s3 = st.str( "hello" );

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
  st["a"]             = st.new_table();
  st["a"][5]          = st.new_table();
  st["a"][5]["world"] = 9;

  table G = st.global_table();

  REQUIRE( ( G["a"][5]["world"] == 9 ) );
  REQUIRE( ( G["a"] == st["a"] ) );
  REQUIRE( ( G["a"][5] == st["a"][5] ) );
  REQUIRE( ( G["a"][5] != st["a"] ) );

  REQUIRE( C.stack_size() == 0 );
}

} // namespace
} // namespace lua
