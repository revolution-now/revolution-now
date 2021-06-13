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

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[lua-state] standard tables" ) {
  C.openlibs();

  table G = st.table.global;
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

  table G = st.table.global;

  REQUIRE( ( G["a"][5]["world"] == 9 ) );
  REQUIRE( ( G["a"] == st["a"] ) );
  REQUIRE( ( G["a"][5] == st["a"][5] ) );
  REQUIRE( ( G["a"][5] != st["a"] ) );
}

} // namespace
} // namespace lua
