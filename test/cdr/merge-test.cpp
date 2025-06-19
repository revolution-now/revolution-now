/****************************************************************
**merge-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-18.
*
* Description: Unit tests for the cdr/merge module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/merge.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace cdr {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[cdr/merge] right_join" ) {
  using namespace ::cdr::literals;

  table l, r, ex;

  auto const f = [&] { return right_join( l, r ); };

  l  = {};
  r  = {};
  ex = {};
  REQUIRE( f() == 0 );
  REQUIRE( l == ex );

  l = {
    "n"_key = null,
    "a"_key = 7,
    "b"_key = "hello",
    "c"_key = 4.2,
    "d"_key =
        table{
          "e"_key = 999,
          "f"_key =
              list{
                8,
                "hello",
                table{
                  "h"_key = 8,
                  "i"_key = "false",
                },
                1.2,
              },
          "g"_key = true,
        },
  };
  r  = l;
  ex = l;
  REQUIRE( f() == 0 );
  REQUIRE( l == ex );

  r["j"]                  = 3;
  r["d"].as<table>()["k"] = 4;
  r["d"].as<table>()["f"].as<list>().push_back( 3.4 );
  r["d"].as<table>()["f"].as<list>()[2].as<table>()["h"] = 9;
  r["d"].as<table>()["f"].as<list>()[2].as<table>()["l"] = false;

  ex = {
    "n"_key = null,
    "a"_key = 7,
    "b"_key = "hello",
    "c"_key = 4.2,
    "d"_key =
        table{
          "e"_key = 999,
          "f"_key =
              list{
                8,
                "hello",
                table{
                  "h"_key = 8,
                  "i"_key = "false",
                  "l"_key = false,
                },
                1.2,
                3.4,
              },
          "g"_key = true,
          "k"_key = 4,
        },
    "j"_key = 3,
  };
  REQUIRE( f() == 4 );
  REQUIRE( l == ex );

  l["d"].as<table>()["f"].as<list>().push_back( "abc" );
  ex = l;
  REQUIRE( f() == 0 );
  REQUIRE( l == ex );
}

} // namespace
} // namespace cdr
