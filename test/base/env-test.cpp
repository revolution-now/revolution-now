/****************************************************************
**env-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Unit tests for the base/env module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/env.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
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
} // namespace base
