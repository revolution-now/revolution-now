/****************************************************************
**math.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Unit tests for the math module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "math.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

TEST_CASE( "[math] modulus" ) {
  REQUIRE( ::rn::modulus( 7, 3 ) == 1 );
  REQUIRE( ::rn::modulus( 7, -3 ) == 1 );
  REQUIRE( ::rn::modulus( -7, 3 ) == 2 );
  REQUIRE( ::rn::modulus( -7, -3 ) == 2 );
}

} // namespace
