/****************************************************************
**map-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Unit tests for the src/map-square.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/map-square.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[map-square] is_land, is_water" ) {
  MapSquare square;

  square = MapSquare{ .terrain = e_terrain::ocean };
  REQUIRE_FALSE( is_land( square ) );
  REQUIRE( is_water( square ) );
  REQUIRE( surface_type( square ) == e_surface::water );

  square = MapSquare{ .terrain = e_terrain::tundra };
  REQUIRE( is_land( square ) );
  REQUIRE_FALSE( is_water( square ) );
  REQUIRE( surface_type( square ) == e_surface::land );
}

} // namespace
} // namespace rn
