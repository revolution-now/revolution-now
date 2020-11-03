/****************************************************************
**rnl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-02.
*
* Description: Unit tests for the rnl language.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "rnl/testing.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[rnl] some test" ) {
  int x = 4;
  REQUIRE( x == 4 );
}

} // namespace
} // namespace rn