/****************************************************************
**harbor-units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
* Description: Unit tests for the src/harbor-units.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Testing.
#include "test/fake/world.hpp"

// Under test.
#include "src/harbor-units.hpp"

// Revolution Now
#include "src/gs-units.hpp"
#include "src/player.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

struct HarborUnitsWorld : testing::World {
  using Base = testing::World;
  HarborUnitsWorld() : Base() {
    MapSquare const   O = make_ocean();
    MapSquare const   S = make_sea_lane();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        O, O, O, O, O, O, O, O, S, S, //
        O, O, O, O, O, O, O, O, S, S, //
        O, O, L, L, L, L, L, O, S, S, //
        O, O, L, L, L, L, L, O, S, S, //
        O, O, L, L, L, L, O, O, S, S, //
        O, O, L, L, L, L, O, O, S, S, //
        O, O, L, L, L, L, L, O, S, S, //
        O, O, O, O, O, O, O, O, S, S, //
        O, O, O, O, O, O, O, O, S, S, //
        O, O, O, O, O, O, O, O, S, S, //
    };
    build_map( std::move( tiles ) );
    add_player( e_nation::dutch );
    add_player( e_nation::french );
  }
};

TEST_CASE( "[harbor-units] is_unit_inbound" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] is_unit_outbound" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] is_unit_in_port" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_on_dock" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_in_port" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_inbound" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_outbound" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] unit_sail_to_new_world" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] unit_sail_to_harbor" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] unit_move_to_port" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE( "[harbor-units] advance_unit_on_high_seas" ) {
  HarborUnitsWorld world;
  // TODO
}

TEST_CASE(
    "[harbor-units] sail to new world with foreign unit" ) {
  HarborUnitsWorld world;
  // TODO
}

} // namespace
} // namespace rn
