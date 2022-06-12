/****************************************************************
**production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-06.
*
* Description: Unit tests for the src/production.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/production.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/gs-terrain.hpp"
#include "src/gs-units.hpp"
#include "src/player.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_player( e_nation::dutch ); }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const S = make_sea_lane();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3_w );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[production] crosses production" ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( Coord( 1_x, 1_y ) );
  Player& player = W.dutch();

  auto crosses = [&] {
    ColonyProduction pr = production_for_colony(
        W.terrain(), W.units(), player, colony );
    return pr.crosses;
  };

  // Baseline.
  REQUIRE( crosses() == 1 );

  // With church.
  colony.add_building( e_colony_building::church );
  REQUIRE( crosses() == 2 );

  // With free colonist working in church.
  W.add_unit_indoors( colony.id(), e_indoor_job::crosses );
  REQUIRE( crosses() == 2 + 3 );

  // With free colonist and firebrand preacher working in church.
  W.add_unit_indoors( colony.id(), e_indoor_job::crosses,
                      e_unit_type::firebrand_preacher );
  REQUIRE( crosses() == 2 + 3 + 6 );

  // With free colonist and firebrand preacher working in cathe-
  // dral.
  colony.add_building( e_colony_building::cathedral );
  REQUIRE( crosses() == 3 + ( 3 + 6 ) * 2 );

  // Taking away the church should have no effect because the
  // cathedral should override it.
  colony.rm_building( e_colony_building::church );
  REQUIRE( crosses() == 3 + ( 3 + 6 ) * 2 );

  // With free colonist and firebrand preacher working in cathe-
  // dral with William Penn.
  player.fathers.has[e_founding_father::william_penn] = true;
  // Should be a 50% increase (rounded up) applied only to the
  // units' production and not the base production.
  REQUIRE( crosses() ==
           ( 3 + ( 3 + 6 ) * 2 ) + ( ( 3 + 6 ) * 2 ) / 2 );

  // All colonies constructed in this test should be valid.
  REQUIRE( W.validate_colonies() == base::valid );
}

} // namespace
} // namespace rn
