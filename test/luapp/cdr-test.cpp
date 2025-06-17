/****************************************************************
**cdr-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Unit tests for the luapp/cdr module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/cdr.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/pretty.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace lua {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
LUA_TEST_CASE( "[luapp/cdr] cdr to lua" ) {
  LuaToCdrConverter c( st );
  string expected;

  using namespace cdr::literals;

  SECTION( "nil" ) {
    cdr::value const v = cdr::null;
    any const a        = c.convert( v );
    expected           = "nil";
    REQUIRE( pretty_print( a ) == expected );
  }

  SECTION( "just int" ) {
    cdr::value const v = 5;
    any const a        = c.convert( v );
    expected           = "5";
    REQUIRE( pretty_print( a ) == expected );
  }

  SECTION( "complex" ) {
    cdr::value const v = cdr::table{
      "foo"_key =
          cdr::table{
            "bar"_key =
                cdr::table{
                  "baz"_key      = "hello",
                  "some_key"_key = false,
                },
            "some_list"_key =
                cdr::list{
                  8,
                  "world",
                  true,
                },
          },
      "another_key"_key = cdr::null,
      "my_key"_key      = 5.4,
      "an_int"_key      = 10,
      "a_str"_key       = "888",
    };

    any const a = c.convert( v );

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
    cdr::value const v = cdr::list{
      "hello",
      5,
      "world",
    };
    any const a = c.convert( v );
    expected    = R"lua({
  [1] = "hello",
  [2] = 5,
  [3] = "world",
})lua";
    REQUIRE( pretty_print( a ) == expected );
  }
}

LUA_TEST_CASE( "[luapp/cdr] cdr list stringify" ) {
  LuaToCdrConverter c( st );

  cdr::value const v = cdr::list{ 6, "hello", 9.7 };

  table tbl = c.convert( v ).as<lua::table>();

  REQUIRE( format( "{}", tbl ) == "list[#3]" );

  tbl[4] = true;
  REQUIRE( format( "{}", tbl ) == "list[#4]" );
}

} // namespace
} // namespace lua
