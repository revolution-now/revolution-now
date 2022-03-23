/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Unit tests for the src/terrain.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/terrain.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[terrain] is_land, is_water" ) {
  e_terrain terrain;

  terrain = e_terrain::ocean;
  REQUIRE_FALSE( is_land( terrain ) );
  REQUIRE( is_water( terrain ) );
  REQUIRE( surface_type( terrain ) == e_surface::water );

  terrain = e_terrain::grassland;
  REQUIRE( is_land( terrain ) );
  REQUIRE_FALSE( is_water( terrain ) );
  REQUIRE( surface_type( terrain ) == e_surface::land );
}

} // namespace
} // namespace rn
