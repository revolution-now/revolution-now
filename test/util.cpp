/****************************************************************
**util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-09-27.
*
* Description: Unit tests for the src/util.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/util.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[env] Set/Get env var" ) {
  REQUIRE( env_var( "abcdefghi" ) == nothing );
  set_env_var( "abcdefghi", "hello" );
  REQUIRE( env_var( "abcdefghi" ) == "hello" );
  set_env_var_if_not_set( "abcdefghi", "world" );
  REQUIRE( env_var( "abcdefghi" ) == "hello" );
  set_env_var( "abcdefghi", "world" );
  REQUIRE( env_var( "abcdefghi" ) == "world" );
  unset_env_var( "abcdefghi" );
  REQUIRE( env_var( "abcdefghi" ) == nothing );

  unset_env_var( "COLUMNS" );
  REQUIRE( os_terminal_columns() == nothing );
  set_env_var( "COLUMNS", "67" );
  REQUIRE( os_terminal_columns() == 67 );
  unset_env_var( "COLUMNS" );
  REQUIRE( os_terminal_columns() == nothing );
}

} // namespace
} // namespace rn
