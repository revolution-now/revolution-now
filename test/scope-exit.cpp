/****************************************************************
**scope-exit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Unit tests for the src/base/scope-exit.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/scope-exit.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[scope-exit] SCOPE_EXIT" ) {
  SECTION( "one statement" ) {
    int x = 0;
    {
      SCOPE_EXIT( x = 5 );
      x = 3;
      REQUIRE( x == 3 );
    }
    REQUIRE( x == 5 );
  }
  SECTION( "block statement" ) {
    int y = 0;
    {
      SCOPE_EXIT( {
        y = 4;
        y = 6;
      } );
      y = 3;
      REQUIRE( y == 3 );
    }
    REQUIRE( y == 6 );
  }
}

TEST_CASE( "[scope-exit] SCOPED_SET" ) {
  int x = 5;
  REQUIRE( x == 5 );
  {
    SCOPED_SET( x, 6 );
    REQUIRE( x == 6 );
    SCOPED_SET( x, 7 );
    REQUIRE( x == 7 );
    {
      SCOPED_SET( x, 8 );
      REQUIRE( x == 8 );
    }
    REQUIRE( x == 7 );
  }
  REQUIRE( x == 5 );
}

} // namespace
} // namespace base
