/****************************************************************
**string.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-28.
*
* Description: Unit tests for the src/base/string.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/string.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[string] trim" ) {
  REQUIRE( trim( "" ) == "" );
  REQUIRE( trim( " " ) == "" );
  REQUIRE( trim( "x" ) == "x" );
  REQUIRE( trim( " x" ) == "x" );
  REQUIRE( trim( "x " ) == "x" );
  REQUIRE( trim( " x " ) == "x" );
  REQUIRE( trim( " bbx " ) == "bbx" );
  REQUIRE( trim( " hello world " ) == "hello world" );
  REQUIRE( trim( "hello world" ) == "hello world" );
  REQUIRE( trim( "one    two three " ) == "one    two three" );
}

} // namespace
} // namespace base
