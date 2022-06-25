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

// config
#include "config/terrain.hpp"

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

TEST_CASE( "[terrain] can_plow" ) {
  REQUIRE( can_plow( e_terrain::grassland ) );
  REQUIRE( can_plow( e_terrain::conifer ) );
  REQUIRE( can_plow( e_terrain::tundra ) );
  REQUIRE( can_plow( e_terrain::boreal ) );
  REQUIRE_FALSE( can_plow( e_terrain::ocean ) );
  REQUIRE_FALSE( can_plow( e_terrain::hills ) );
  REQUIRE_FALSE( can_plow( e_terrain::mountains ) );
  REQUIRE_FALSE( can_plow( e_terrain::arctic ) );
}

TEST_CASE( "[terrain] cleared_forest" ) {
  REQUIRE( cleared_forest( e_terrain::grassland ) == nothing );
  REQUIRE( cleared_forest( e_terrain::conifer ) ==
           e_ground_terrain::grassland );
  REQUIRE( cleared_forest( e_terrain::tundra ) == nothing );
  REQUIRE( cleared_forest( e_terrain::boreal ) ==
           e_ground_terrain::tundra );
  REQUIRE( cleared_forest( e_terrain::ocean ) == nothing );
  REQUIRE( cleared_forest( e_terrain::hills ) == nothing );
  REQUIRE( cleared_forest( e_terrain::mountains ) == nothing );
  REQUIRE( cleared_forest( e_terrain::arctic ) == nothing );
}

TEST_CASE( "[terrain] has_forest" ) {
  REQUIRE_FALSE( has_forest( e_terrain::grassland ) );
  REQUIRE( has_forest( e_terrain::conifer ) );
  REQUIRE_FALSE( has_forest( e_terrain::tundra ) );
  REQUIRE( has_forest( e_terrain::boreal ) );
  REQUIRE_FALSE( has_forest( e_terrain::ocean ) );
  REQUIRE_FALSE( has_forest( e_terrain::hills ) );
  REQUIRE_FALSE( has_forest( e_terrain::mountains ) );
  REQUIRE_FALSE( has_forest( e_terrain::arctic ) );
}

TEST_CASE( "[terrain] can_irrigate" ) {
  REQUIRE( can_irrigate( e_terrain::grassland ) );
  REQUIRE_FALSE( can_irrigate( e_terrain::conifer ) );
  REQUIRE( can_irrigate( e_terrain::tundra ) );
  REQUIRE_FALSE( can_irrigate( e_terrain::boreal ) );
  REQUIRE_FALSE( can_irrigate( e_terrain::ocean ) );
  REQUIRE_FALSE( can_irrigate( e_terrain::hills ) );
  REQUIRE_FALSE( can_irrigate( e_terrain::mountains ) );
  REQUIRE_FALSE( can_irrigate( e_terrain::arctic ) );
}

TEST_CASE( "[terrain] to_ground_terrain" ) {
  REQUIRE( to_ground_terrain( e_terrain::grassland ) ==
           e_ground_terrain::grassland );
  REQUIRE( to_ground_terrain( e_terrain::conifer ) == nothing );
  REQUIRE( to_ground_terrain( e_terrain::tundra ) ==
           e_ground_terrain::tundra );
  REQUIRE( to_ground_terrain( e_terrain::boreal ) == nothing );
  REQUIRE( to_ground_terrain( e_terrain::ocean ) == nothing );
  REQUIRE( to_ground_terrain( e_terrain::hills ) == nothing );
  REQUIRE( to_ground_terrain( e_terrain::mountains ) ==
           nothing );
  REQUIRE( to_ground_terrain( e_terrain::arctic ) ==
           e_ground_terrain::arctic );
}

TEST_CASE( "[terrain] from_ground_terrain" ) {
  REQUIRE( from_ground_terrain( e_ground_terrain::grassland ) ==
           e_terrain::grassland );
  REQUIRE( from_ground_terrain( e_ground_terrain::tundra ) ==
           e_terrain::tundra );
  REQUIRE( from_ground_terrain( e_ground_terrain::arctic ) ==
           e_terrain::arctic );
}

} // namespace
} // namespace rn
