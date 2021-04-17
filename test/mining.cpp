/****************************************************************
**mining.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-26.
*
* Description: Unit tests for the src/mining.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/mining.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[mining] temporary" ) {
  UnitLandProductionInfo info;

  LandSquare  square;
  e_unit_type utype;
  e_commodity comm;

  square = LandSquare{ .surface = e_surface::water };
  utype  = e_unit_type::free_colonist;
  comm   = e_commodity::food;
  info   = production_for_landsquare( square, comm, utype );
  REQUIRE( info.quantity == 0 );
  REQUIRE( info.reason ==
           e_land_production_reason::void_of_commodity );

  square = LandSquare{ .surface = e_surface::land };
  utype  = e_unit_type::free_colonist;
  comm   = e_commodity::food;
  info   = production_for_landsquare( square, comm, utype );
  REQUIRE( info.quantity == 4 );
  REQUIRE( info.reason == e_land_production_reason::ok );
}

} // namespace
} // namespace rn
