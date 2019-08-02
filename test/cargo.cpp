/****************************************************************
**cargo.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-01.
*
* Description: Unit tests for cargo module.
*
*****************************************************************/
// Revolution Now
#include "src/cargo.hpp"

// Catch2
#include "catch2/catch.hpp"

using namespace rn;

namespace {

TEST_CASE( "cargo initially empty" ) {
  CargoHold hold( 6 );
  REQUIRE( hold.count_items() == 0 );
}

} // namespace
