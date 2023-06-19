/****************************************************************
**cli-args.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: Unit tests for the src/base/cli-args.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/cli-args.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename... Arg>
expect<ProgramArguments> cmd( Arg&&... arg ) {
  return parse_args(
      vector<string>{ std::forward<Arg>( arg )... } );
}

TEST_CASE( "[cli-args] happy path" ) {
  auto res = cmd( "hello world", "--yoyo", "--count=444", "test",
                  "one" );
  auto expected = ProgramArguments{
      .key_val_args    = { { "count", "444" } },
      .flag_args       = { "yoyo" },
      .positional_args = { "hello world", "test", "one" },
  };
  REQUIRE( res == expected );
}

TEST_CASE( "[cli-args] empty" ) {
  auto res      = cmd();
  auto expected = ProgramArguments{
      .key_val_args    = {},
      .flag_args       = {},
      .positional_args = {},
  };
  REQUIRE( res == expected );
}

TEST_CASE( "[cli-args] duplicate key/value" ) {
  auto res = cmd( "hello world", "--yoyo", "--count=444", "test",
                  "one", "--count=222" );
  string expected = "duplicate key/value argument `count'.";
  REQUIRE( res == expected );
}

TEST_CASE( "[cli-args] duplicate flag" ) {
  auto res = cmd( "hello world", "--yoyo", "--count=444", "test",
                  "one", "--yoyo" );
  string expected = "duplicate flag argument `yoyo'.";
  REQUIRE( res == expected );
}

TEST_CASE( "[cli-args] invalid argument" ) {
  auto   res = cmd( "hello world", "--yoyo", "--count=444=1",
                    "test", "one" );
  string expected = "invalid argument `count=444=1'.";
  REQUIRE( res == expected );
}

} // namespace
} // namespace base
