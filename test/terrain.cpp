/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-03.
*
* Description: Unit tests for terrain module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "coord.hpp"
#include "terrain.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::Delta );
FMT_TO_CATCH( ::rn::Coord );
FMT_TO_CATCH( ::rn::e_crust );

namespace rn {
namespace {

using namespace std;
using namespace rn;

// using Catch::Contains;
// using Catch::UnorderedEquals;

TEST_CASE( "[terrain] generate unit testing land" ) {
  generate_unittest_terrain();

  REQUIRE( world_size_tiles() == Delta{ 10_w, 10_h } );
  REQUIRE( world_size_pixels() == Delta{ 320_w, 320_h } );
  REQUIRE( world_rect_tiles() == Rect{ 0_x, 0_y, 10_w, 10_h } );
  REQUIRE( world_rect_pixels() ==
           Rect{ 0_x, 0_y, 320_w, 320_h } );

  REQUIRE( square_exists( { 0_x, 0_y } ) );
  REQUIRE( square_exists( { 0_x, 9_y } ) );
  REQUIRE( square_exists( { 9_x, 0_y } ) );
  REQUIRE( square_exists( { 9_x, 9_y } ) );

  REQUIRE_FALSE( square_exists( { -1_x, -1_y } ) );
  REQUIRE_FALSE( square_exists( { 0_x, 10_y } ) );
  REQUIRE_FALSE( square_exists( { 10_x, 0_y } ) );
  REQUIRE_FALSE( square_exists( { 10_x, 10_y } ) );

  REQUIRE( square_at( { 0_x, 0_y } ).crust == e_crust::water );
  REQUIRE( square_at( { 1_x, 1_y } ).crust == e_crust::water );
  REQUIRE( square_at( { 8_x, 8_y } ).crust == e_crust::water );
  REQUIRE( square_at( { 9_x, 9_y } ).crust == e_crust::water );

  REQUIRE( square_at( { 1_x, 1_y } ).crust == e_crust::water );
  REQUIRE( square_at( { 2_x, 2_y } ).crust == e_crust::land );
  REQUIRE( square_at( { 7_x, 7_y } ).crust == e_crust::land );
  REQUIRE( square_at( { 5_x, 6_y } ).crust == e_crust::land );

  REQUIRE_FALSE( terrain_is_land( { 0_x, 0_y } ) );
  REQUIRE_FALSE( terrain_is_land( { 1_x, 0_y } ) );
  REQUIRE_FALSE( terrain_is_land( { 1_x, 1_y } ) );
  REQUIRE_FALSE( terrain_is_land( { 1_x, 8_y } ) );
  REQUIRE_FALSE( terrain_is_land( { 9_x, 8_y } ) );
  REQUIRE( terrain_is_land( { 4_x, 3_y } ) );
}

} // namespace
} // namespace rn
