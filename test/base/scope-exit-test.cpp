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
#include "test/testing.hpp"

// Under test.
#include "src/base/scope-exit.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[scope-exit] SCOPE_EXIT" ) {
  SECTION( "one statement" ) {
    int x = 0;
    {
      SCOPE_EXIT { x = 5; };
      x = 3;
      REQUIRE( x == 3 );
    }
    REQUIRE( x == 5 );
  }
  SECTION( "block statement" ) {
    int y = 0;
    {
      SCOPE_EXIT {
        y = 4;
        y = 6;
      };
      y = 3;
      REQUIRE( y == 3 );
    }
    REQUIRE( y == 6 );
  }
}

TEST_CASE( "[scope-exit] SCOPED_SET_AND_RESTORE" ) {
  int x = 5;
  REQUIRE( x == 5 );
  {
    SCOPED_SET_AND_RESTORE( x, 6 );
    REQUIRE( x == 6 );
    SCOPED_SET_AND_RESTORE( x, 7 );
    REQUIRE( x == 7 );
    {
      SCOPED_SET_AND_RESTORE( x, 8 );
      REQUIRE( x == 8 );
    }
    REQUIRE( x == 7 );
  }
  REQUIRE( x == 5 );
}

TEST_CASE( "[scope-exit] SCOPED_SET_AND_CHANGE" ) {
  int x = 5;
  REQUIRE( x == 5 );
  {
    SCOPED_SET_AND_CHANGE( x, 6, 7 );
    REQUIRE( x == 6 );
    SCOPED_SET_AND_CHANGE( x, 7, 8 );
    REQUIRE( x == 7 );
    {
      SCOPED_SET_AND_CHANGE( x, 8, 9 );
      REQUIRE( x == 8 );
    }
    REQUIRE( x == 9 );
  }
  REQUIRE( x == 7 );
}

} // namespace
} // namespace base
