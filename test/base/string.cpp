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
#include "test/testing.hpp"

// Under test.
#include "src/base/string.hpp"

// Must be last.
#include "test/catch-common.hpp"

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

TEST_CASE( "[string] capitalize_initials" ) {
  REQUIRE( capitalize_initials( "" ) == "" );
  REQUIRE( capitalize_initials( " " ) == " " );
  REQUIRE( capitalize_initials( "   " ) == "   " );
  REQUIRE( capitalize_initials( "  a" ) == "  A" );
  REQUIRE( capitalize_initials( "  A" ) == "  A" );
  REQUIRE( capitalize_initials( "a" ) == "A" );
  REQUIRE( capitalize_initials( "A" ) == "A" );
  REQUIRE( capitalize_initials( "A" ) == "A" );
  REQUIRE( capitalize_initials( "a " ) == "A " );
  REQUIRE( capitalize_initials( "A " ) == "A " );
  REQUIRE( capitalize_initials( "A " ) == "A " );
  REQUIRE( capitalize_initials( "this is a test" ) ==
           "This Is A Test" );
  REQUIRE( capitalize_initials( "this Is" ) == "This Is" );
  REQUIRE( capitalize_initials( " this Is" ) == " This Is" );
}

} // namespace
} // namespace base
