/****************************************************************
**lcr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-23.
*
* Description: Unit tests for the src/lcr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Revolution Now
#include "gs-terrain.hpp"

// Under test.
#include "src/lcr.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[test/lcr] has_lost_city_rumor" ) {
  TerrainState terrain_state;
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );

  REQUIRE_FALSE( has_lost_city_rumor( terrain_state, Coord{} ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.lost_city_rumor = true;
  REQUIRE( has_lost_city_rumor( terrain_state, Coord{} ) );
}

} // namespace
} // namespace rn
