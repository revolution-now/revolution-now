/****************************************************************
**pretty-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Unit tests for the luapp/pretty module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/pretty.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/any.hpp"
#include "src/luapp/state.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace lua {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
LUA_TEST_CASE( "[luapp/pretty] pretty_print" ) {
  string expected;

  SECTION( "nil" ) {
    any const a = st.as<any>( lua::nil );
    expected    = "nil";
    REQUIRE( pretty_print( a ) == expected );
  }

  SECTION( "just int" ) {
    any const a = st.as<any>( 5 );
    expected    = "5";
    REQUIRE( pretty_print( a ) == expected );
  }

  SECTION( "complex" ) {
    table tbl                     = st.table.create();
    tbl["foo"]                    = st.table.create();
    tbl["foo"]["bar"]             = st.table.create();
    tbl["foo"]["bar"]["baz"]      = "hello";
    tbl["foo"]["bar"]["some_key"] = false;
    tbl["foo"]["some_list"]       = st.table.create();
    tbl["foo"]["some_list"][1]    = 8;
    tbl["foo"]["some_list"][2]    = "world";
    tbl["foo"]["some_list"][3]    = true;
    tbl["another_key"]            = lua::nil;
    tbl["my_key"]                 = 5.4;
    tbl["an_int"]                 = 10;
    tbl["a_str"]                  = "888";

    any const a = tbl;

    expected = R"lua({
  a_str = "888",
  an_int = 10,
  foo = {
    bar = {
      baz = "hello",
      some_key = false,
    },
    some_list = {
      [1] = 8,
      [2] = "world",
      [3] = true,
    },
  },
  my_key = 5.4,
})lua";
    REQUIRE( pretty_print( a ) == expected );
  }

  SECTION( "list at top" ) {
    table tbl   = st.table.create();
    tbl[1]      = "hello";
    tbl[2]      = 5;
    tbl[3]      = "world";
    any const a = tbl;
    expected    = R"lua({
  [1] = "hello",
  [2] = 5,
  [3] = "world",
})lua";
    REQUIRE( pretty_print( a ) == expected );
  }
}

} // namespace
} // namespace lua
