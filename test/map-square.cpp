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

TEST_CASE( "[map-square] movement_points_required" ) {
  MapSquare ocean{ .terrain = e_terrain::ocean };
  MapSquare grassland{ .terrain = e_terrain::grassland };
  MapSquare mountains{ .terrain = e_terrain::mountains };

  MapSquare ocean_with_road{ .terrain = e_terrain::ocean,
                             .road    = true };
  MapSquare grassland_with_road{ .terrain = e_terrain::grassland,
                                 .road    = true };
  MapSquare mountains_with_road{ .terrain = e_terrain::mountains,
                                 .road    = true };

  reference_wrapper<MapSquare> src = ocean;
  reference_wrapper<MapSquare> dst = ocean;

  MovementPoints expected;
  auto           f = movement_points_required;

  src      = ocean;
  dst      = ocean;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland;
  dst      = ocean;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = ocean;
  dst      = grassland;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland;
  dst      = grassland;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = mountains;
  dst      = grassland;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland;
  dst      = mountains;
  expected = 3;
  REQUIRE( f( src, dst ) == expected );

  src      = mountains;
  dst      = mountains;
  expected = 3;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland_with_road;
  dst      = grassland;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland;
  dst      = grassland_with_road;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland_with_road;
  dst      = grassland_with_road;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );

  src      = mountains_with_road;
  dst      = grassland;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = mountains;
  dst      = grassland_with_road;
  expected = 1;
  REQUIRE( f( src, dst ) == expected );

  src      = mountains_with_road;
  dst      = grassland_with_road;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );

  src      = grassland_with_road;
  dst      = mountains;
  expected = 3;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland;
  dst      = mountains_with_road;
  expected = 3;
  REQUIRE( f( src, dst ) == expected );

  src      = grassland_with_road;
  dst      = mountains_with_road;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );

  src      = mountains_with_road;
  dst      = mountains;
  expected = 3;
  REQUIRE( f( src, dst ) == expected );

  src      = mountains;
  dst      = mountains_with_road;
  expected = 3;
  REQUIRE( f( src, dst ) == expected );

  src      = mountains_with_road;
  dst      = mountains_with_road;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );
}

} // namespace
} // namespace rn
