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

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[map-square] is_land, is_water" ) {
  MapSquare square;

  square = map_square_for_terrain( e_terrain::ocean );
  REQUIRE_FALSE( is_land( square ) );
  REQUIRE( is_water( square ) );
  REQUIRE( surface_type( square ) == e_surface::water );

  square = map_square_for_terrain( e_terrain::tundra );
  REQUIRE( is_land( square ) );
  REQUIRE_FALSE( is_water( square ) );
  REQUIRE( surface_type( square ) == e_surface::land );

  square = map_square_for_terrain( e_terrain::hills );
  REQUIRE( is_land( square ) );
  REQUIRE_FALSE( is_water( square ) );
  REQUIRE( surface_type( square ) == e_surface::land );
}

TEST_CASE( "[map-square] movement_points_required" ) {
  MapSquare ocean = map_square_for_terrain( e_terrain::ocean );
  MapSquare grassland =
      map_square_for_terrain( e_terrain::grassland );
  MapSquare mountains =
      map_square_for_terrain( e_terrain::mountains );
  MapSquare grassland_with_road =
      map_square_for_terrain( e_terrain::grassland );
  grassland_with_road.road = true;
  MapSquare mountains_with_road =
      map_square_for_terrain( e_terrain::mountains );
  mountains_with_road.road = true;

  MapSquare grassland_with_road_and_river =
      map_square_for_terrain( e_terrain::grassland );
  grassland_with_road_and_river.road = true;
  MapSquare mountains_with_river =
      map_square_for_terrain( e_terrain::mountains );
  mountains_with_river.river = e_river::major;

  reference_wrapper<MapSquare const> src = ocean;
  reference_wrapper<MapSquare const> dst = ocean;

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

  src      = grassland_with_road;
  dst      = grassland_with_road_and_river;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );

  src      = grassland_with_road_and_river;
  dst      = grassland_with_road;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );

  src      = grassland_with_road_and_river;
  dst      = grassland_with_road_and_river;
  expected = MovementPoints::_1_3();
  REQUIRE( f( src, dst ) == expected );
}

TEST_CASE( "[map-square] effective_terrain" ) {
  MapSquare square;
  e_terrain expected;

  square = MapSquare{
      .surface         = e_surface::water,
      .ground_resource = e_natural_resource::fish,
      .sea_lane        = true,
  };
  expected = e_terrain::ocean;
  REQUIRE( effective_terrain( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .road    = true,
  };
  expected = e_terrain::savannah;
  REQUIRE( effective_terrain( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::forest,
      .road    = true,
  };
  expected = e_terrain::tropical;
  REQUIRE( effective_terrain( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::hills,
      .road    = true,
  };
  expected = e_terrain::hills;
  REQUIRE( effective_terrain( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::mountains,
      .road    = true,
  };
  expected = e_terrain::mountains;
  REQUIRE( effective_terrain( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::arctic,
      .road    = true,
  };
  expected = e_terrain::arctic;
  REQUIRE( effective_terrain( square ) == expected );
}

TEST_CASE( "[map-square] can_plow" ) {
  MapSquare square;
  bool      expected;

  square = MapSquare{
      .surface         = e_surface::water,
      .ground_resource = e_natural_resource::fish,
  };
  expected = false;
  REQUIRE( can_plow( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .road    = true,
  };
  expected = true;
  REQUIRE( can_plow( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::forest,
      .road    = true,
  };
  expected = true;
  REQUIRE( can_plow( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::hills,
  };
  expected = false;
  REQUIRE( can_plow( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::mountains,
  };
  expected = false;
  REQUIRE( can_plow( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::arctic,
  };
  expected = false;
  REQUIRE( can_plow( square ) == expected );
}

TEST_CASE( "[map-square] has_forest" ) {
  MapSquare square;
  bool      expected;

  square = MapSquare{
      .surface         = e_surface::water,
      .forest_resource = e_natural_resource::fish,
  };
  expected = false;
  REQUIRE( has_forest( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .road    = true,
  };
  expected = false;
  REQUIRE( has_forest( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::forest,
      .road    = true,
  };
  expected = true;
  REQUIRE( has_forest( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::hills,
  };
  expected = false;
  REQUIRE( has_forest( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::mountains,
  };
  expected = false;
  REQUIRE( has_forest( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::arctic,
  };
  expected = false;
  REQUIRE( has_forest( square ) == expected );
}

TEST_CASE( "[map-square] clear_forest" ) {
  MapSquare square, expected;

  square = expected = MapSquare{
      .surface         = e_surface::land,
      .ground          = e_ground_terrain::savannah,
      .overlay         = e_land_overlay::forest,
      .forest_resource = e_natural_resource::beaver,
  };
  expected.overlay = nothing;
  REQUIRE( effective_terrain( expected ) ==
           e_terrain::savannah );
  REQUIRE( effective_terrain( square ) == e_terrain::tropical );
  clear_forest( square );
  REQUIRE( effective_terrain( square ) == e_terrain::savannah );
  REQUIRE( square == expected );

  square = expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::desert,
      .overlay = e_land_overlay::forest,
  };
  expected.overlay = nothing;
  REQUIRE( effective_terrain( expected ) == e_terrain::desert );
  REQUIRE( effective_terrain( square ) == e_terrain::scrub );
  clear_forest( square );
  REQUIRE( effective_terrain( square ) == e_terrain::desert );
  REQUIRE( square == expected );
}

TEST_CASE( "[map-square] can_irrigate" ) {
  MapSquare square;
  bool      expected;

  square = MapSquare{
      .surface         = e_surface::water,
      .ground_resource = e_natural_resource::fish,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .road    = true,
  };
  expected = true;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::forest,
      .road    = true,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::hills,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::mountains,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::arctic,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );
}

TEST_CASE( "[map-square] irrigate" ) {
  MapSquare square, expected;

  square = expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
  };
  expected.irrigation = true;
  irrigate( square );
  REQUIRE( square == expected );

  square = expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::desert,
  };
  expected.irrigation = true;
  irrigate( square );
  REQUIRE( square == expected );
}

TEST_CASE( "[map-square] map_square_for_terrain" ) {
  MapSquare square, expected;

  square   = map_square_for_terrain( e_terrain::ocean );
  expected = MapSquare{
      .surface = e_surface::water,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::arctic );
  expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::arctic,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::grassland );
  expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::grassland,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::conifer );
  expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::grassland,
      .overlay = e_land_overlay::forest,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::prairie );
  expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::prairie,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::broadleaf );
  expected = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::prairie,
      .overlay = e_land_overlay::forest,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::hills );
  expected = MapSquare{
      .surface = e_surface::land,
      .overlay = e_land_overlay::hills,
  };
  REQUIRE( square == expected );

  square   = map_square_for_terrain( e_terrain::mountains );
  expected = MapSquare{
      .surface = e_surface::land,
      .overlay = e_land_overlay::mountains,
  };
  REQUIRE( square == expected );
}

TEST_CASE( "[map-square] effective_resource" ) {
  MapSquare          square;
  e_natural_resource expected;

  square = MapSquare{};
  REQUIRE( effective_resource( square ) == nothing );

  square = MapSquare{
      .overlay = e_land_overlay::forest,
  };
  expected = e_natural_resource::fish;
  REQUIRE( effective_resource( square ) == expected );

  square = MapSquare{
      .ground_resource = e_natural_resource::fish,
  };
  expected = e_natural_resource::fish;
  REQUIRE( effective_resource( square ) == expected );

  square = MapSquare{
      .overlay         = e_land_overlay::forest,
      .ground_resource = e_natural_resource::cotton,
  };
  REQUIRE( effective_resource( square ) == nothing );

  square = MapSquare{
      .forest_resource = e_natural_resource::beaver,
  };
  REQUIRE( effective_resource( square ) == nothing );

  square = MapSquare{
      .overlay         = e_land_overlay::forest,
      .forest_resource = e_natural_resource::beaver,
  };
  expected = e_natural_resource::beaver;
  REQUIRE( effective_resource( square ) == expected );

  square = MapSquare{
      .ground_resource = e_natural_resource::cotton,
      .forest_resource = e_natural_resource::beaver,
  };
  expected = e_natural_resource::cotton;
  REQUIRE( effective_resource( square ) == expected );

  square = MapSquare{
      .overlay         = e_land_overlay::forest,
      .ground_resource = e_natural_resource::cotton,
      .forest_resource = e_natural_resource::beaver,
  };
  expected = e_natural_resource::beaver;
  REQUIRE( effective_resource( square ) == expected );
}

} // namespace
} // namespace rn
