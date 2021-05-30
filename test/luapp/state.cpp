/****************************************************************
**state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: Unit tests for the src/luapp/state.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/state.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace luapp {
namespace {

using namespace std;

TEST_CASE( "[state] some test" ) {
  int x = 4;
  REQUIRE( x == 4 );

  SECTION( "even" ) { REQUIRE( x % 2 == 0 ); }

  SECTION( "non negative" ) { REQUIRE( x >= 0 ); }
}

} // namespace
} // namespace luapp
