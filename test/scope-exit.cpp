/****************************************************************
**scope-exit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Unit tests for the src/scope-exit.* module.
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

TEST_CASE( "[scope-exit] some test" ) {
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

} // namespace
} // namespace base
