/****************************************************************
**damaged.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-17.
*
* Description: Unit tests for the src/damaged.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/damaged.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/ref.hpp"

// base
#include "refl/to-str.hpp"

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
  World() : Base() {
    add_player( e_nation::french );
    add_player( e_nation::spanish );
    set_default_player( e_nation::french );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, _, L, _, L, //
        L, _, L, _, L, _, L, _, //
        _, L, _, L, _, L, _, L, //
        L, _, L, _, L, _, L, _, //
        _, L, _, L, _, L, _, L, //
        L, _, L, _, L, _, L, _, //
        _, L, _, L, _, L, _, L, //
        L, _, L, _, L, _, L, _, //
    };
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[damaged] find_repair_port_for_ship" ) {
  World W;

  maybe<ShipRepairPort_t> expected      = {};
  Coord const             ship_location = { .x = 4, .y = 4 };

  auto f = [&] {
    return find_repair_port_for_ship( W.ss(), W.default_nation(),
                                      ship_location );
  };

  // No colonies.
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // One foreign colony with no drydock.
  Colony& colony_spanish_1 =
      W.add_colony( { .x = 3, .y = 4 }, e_nation::spanish );
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // One foreign colony with drydock.
  colony_spanish_1.buildings[e_colony_building::drydock] = true;
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // One foreign colony with shipyard.
  colony_spanish_1.buildings[e_colony_building::shipyard] = true;
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // One friendly colony with no drydock.
  Colony& colony_1 =
      W.add_colony( { .x = 1, .y = 2 }, e_nation::french );
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // Two friendly colonies with no drydock.
  Colony& colony_2 =
      W.add_colony( { .x = 0, .y = 0 }, e_nation::french );
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // Two friendly colonies, farther one with drydock.
  colony_2.buildings[e_colony_building::drydock] = true;
  expected = ShipRepairPort::colony{ .id = colony_2.id };
  REQUIRE( f() == expected );

  // Two friendly colonies, both with drydock.
  colony_1.buildings[e_colony_building::drydock] = true;
  colony_2.buildings[e_colony_building::drydock] = true;
  expected = ShipRepairPort::colony{ .id = colony_1.id };
  REQUIRE( f() == expected );

  // Two friendly colonies, one with only shipyard.
  colony_1.buildings[e_colony_building::drydock]  = false;
  colony_2.buildings[e_colony_building::drydock]  = false;
  colony_1.buildings[e_colony_building::shipyard] = true;
  expected = ShipRepairPort::colony{ .id = colony_1.id };
  REQUIRE( f() == expected );

  // Two friendly colonies, both with only shipyard.
  colony_2.buildings[e_colony_building::shipyard] = true;
  expected = ShipRepairPort::colony{ .id = colony_1.id };
  REQUIRE( f() == expected );

  // Three friendly colonies, new one with drydock.
  Colony& colony_3 =
      W.add_colony( { .x = 5, .y = 4 }, e_nation::french );
  colony_3.buildings[e_colony_building::drydock] = true;
  expected = ShipRepairPort::colony{ .id = colony_3.id };
  REQUIRE( f() == expected );

  // Remove third colony's drydock.
  colony_3.buildings[e_colony_building::drydock] = false;
  expected = ShipRepairPort::colony{ .id = colony_1.id };
  REQUIRE( f() == expected );

  // Remove first colony's shipyard.
  colony_1.buildings[e_colony_building::shipyard] = false;
  expected = ShipRepairPort::colony{ .id = colony_2.id };
  REQUIRE( f() == expected );

  // Remove second colony's shipyard.
  colony_2.buildings[e_colony_building::shipyard] = false;
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // Declare independence.
  W.declare_independence();
  expected = nothing;
  REQUIRE( f() == expected );

  // Re-add first colony's shipyard.
  colony_1.buildings[e_colony_building::shipyard] = true;
  expected = ShipRepairPort::colony{ .id = colony_1.id };
  REQUIRE( f() == expected );
}

TEST_CASE( "[damaged] damaged_ship_message" ) {
  World  W;
  string expected;
  int    turns = 0;

  auto f = [&] { return damaged_ship_message( turns ); };

  turns = 1;
  expected =
      "This ship is [damaged] and has [one] turn remaining "
      "until it is repaired.";
  REQUIRE( f() == expected );

  turns = 2;
  expected =
      "This ship is [damaged] and has [two] turns remaining "
      "until it is repaired.";
  REQUIRE( f() == expected );

  turns = 5;
  expected =
      "This ship is [damaged] and has [five] turns remaining "
      "until it is repaired.";
  REQUIRE( f() == expected );

  turns = 10;
  expected =
      "This ship is [damaged] and has [10] turns remaining "
      "until it is repaired.";
  REQUIRE( f() == expected );
}

TEST_CASE( "[damaged] repair_turn_count_for_unit" ) {
  ShipRepairPort_t port;

  auto f = [&]( e_unit_type type ) {
    return repair_turn_count_for_unit( port, type );
  };

  port = ShipRepairPort::european_harbor{};
  REQUIRE( f( e_unit_type::caravel ) == 2 );
  REQUIRE( f( e_unit_type::merchantman ) == 6 );
  REQUIRE( f( e_unit_type::galleon ) == 10 );
  REQUIRE( f( e_unit_type::privateer ) == 8 );
  REQUIRE( f( e_unit_type::frigate ) == 12 );
  REQUIRE( f( e_unit_type::man_o_war ) == 14 );

  port = ShipRepairPort::colony{};
  REQUIRE( f( e_unit_type::caravel ) == 0 );
  REQUIRE( f( e_unit_type::merchantman ) == 2 );
  REQUIRE( f( e_unit_type::galleon ) == 4 );
  REQUIRE( f( e_unit_type::privateer ) == 3 );
  REQUIRE( f( e_unit_type::frigate ) == 5 );
  REQUIRE( f( e_unit_type::man_o_war ) == 7 );
}

} // namespace
} // namespace rn
