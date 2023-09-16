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
#include "ss/unit-composer.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

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

  maybe<ShipRepairPort> expected      = {};
  Coord const           ship_location = { .x = 4, .y = 4 };

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

// This tests that if a ship is damaged in the port of a colony
// that has a drydock that the algo will find the colony it is in
// for repair instead of sending it elsewhere.
TEST_CASE( "[damaged] find_repair_port_for_ship from land" ) {
  World W;

  maybe<ShipRepairPort> expected      = {};
  Coord const           ship_location = { .x = 2, .y = 3 };

  auto f = [&] {
    return find_repair_port_for_ship( W.ss(), W.default_nation(),
                                      ship_location );
  };

  // No colonies.
  expected = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // Colocated colony with no drydock.
  Colony& colony_1 = W.add_colony( ship_location );
  expected         = ShipRepairPort::european_harbor{};
  REQUIRE( f() == expected );

  // Non-colocated colony with drydock.
  Colony& colony_2 = W.add_colony( { .x = 5, .y = 4 } );
  colony_2.buildings[e_colony_building::drydock] = true;
  expected = ShipRepairPort::colony{ .id = colony_2.id };
  REQUIRE( f() == expected );

  // Colocated colony with drydock.
  colony_1.buildings[e_colony_building::drydock] = true;
  expected = ShipRepairPort::colony{ .id = colony_1.id };
  REQUIRE( f() == expected );
}

TEST_CASE( "[damaged] ship_still_damaged_message" ) {
  World  W;
  string expected;
  int    turns = 0;

  auto f = [&] { return ship_still_damaged_message( turns ); };

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
  ShipRepairPort port;

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

TEST_CASE( "[damaged] ship_damaged_no_port_message" ) {
  World                 W;
  string                expected;
  e_ship_damaged_reason reason = {};

  auto f = [&]( Unit const& ship ) {
    return ship_damaged_no_port_message( ship.nation(),
                                         ship.type(), reason );
  };

  reason = e_ship_damaged_reason::battle;
  expected =
      "[French] [Privateer] damaged in battle! As there are no "
      "available repair ports, the ship has been lost.";
  REQUIRE( f( W.add_unit_on_map(
               e_unit_type::privateer, { .x = 0, .y = 0 },
               e_nation::french ) ) == expected );

  reason = e_ship_damaged_reason::battle;
  expected =
      "[Spanish] [Galleon] damaged in battle! As there are no "
      "available repair ports, the ship has been lost.";
  REQUIRE( f( W.add_unit_on_map(
               e_unit_type::galleon, { .x = 0, .y = 0 },
               e_nation::spanish ) ) == expected );

  reason = e_ship_damaged_reason::colony_abandoned;
  expected =
      "[French] [Privateer] damaged during colony collapse! As "
      "there are no available repair ports, the ship has been "
      "lost.";
  REQUIRE( f( W.add_unit_on_map(
               e_unit_type::privateer, { .x = 0, .y = 0 },
               e_nation::french ) ) == expected );

  reason = e_ship_damaged_reason::colony_abandoned;
  expected =
      "[Spanish] [Galleon] damaged during colony collapse! As "
      "there are no available repair ports, the ship has been "
      "lost.";
  REQUIRE( f( W.add_unit_on_map(
               e_unit_type::galleon, { .x = 0, .y = 0 },
               e_nation::spanish ) ) == expected );

  reason = e_ship_damaged_reason::colony_starved;
  expected =
      "[French] [Privateer] damaged during colony collapse! As "
      "there are no available repair ports, the ship has been "
      "lost.";
  REQUIRE( f( W.add_unit_on_map(
               e_unit_type::privateer, { .x = 0, .y = 0 },
               e_nation::french ) ) == expected );

  reason = e_ship_damaged_reason::colony_starved;
  expected =
      "[Spanish] [Galleon] damaged during colony collapse! As "
      "there are no available repair ports, the ship has been "
      "lost.";
  REQUIRE( f( W.add_unit_on_map(
               e_unit_type::galleon, { .x = 0, .y = 0 },
               e_nation::spanish ) ) == expected );
}

TEST_CASE( "[damaged] ship_damaged_message" ) {
  World                 W;
  string                expected;
  ShipRepairPort        port;
  e_ship_damaged_reason reason = {};

  auto f = [&]( Unit const& ship ) {
    return ship_damaged_message( W.ss(), ship.nation(),
                                 ship.type(), reason, port );
  };

  SECTION( "battle" ) {
    reason = e_ship_damaged_reason::battle;
    port   = ShipRepairPort::european_harbor{};
    expected =
        "[French] [Privateer] damaged in battle! Ship sent to "
        "[La Rochelle] for repairs.";
    REQUIRE( f( W.add_unit_on_map(
                 e_unit_type::privateer, { .x = 0, .y = 0 },
                 e_nation::french ) ) == expected );

    Colony& colony = W.add_colony( { .x = 1, .y = 0 } );
    colony.name    = "some colony";
    reason         = e_ship_damaged_reason::battle;
    port           = ShipRepairPort::colony{ .id = colony.id };
    expected =
        "[Spanish] [Man-O-War] damaged in battle! Ship sent to "
        "[some colony] for repairs.";
    REQUIRE( f( W.add_unit_on_map(
                 e_unit_type::man_o_war, { .x = 0, .y = 0 },
                 e_nation::spanish ) ) == expected );
  }

  SECTION( "colony_abandoned" ) {
    reason = e_ship_damaged_reason::colony_abandoned;
    port   = ShipRepairPort::european_harbor{};
    expected =
        "[French] [Privateer] damaged during colony collapse! "
        "Ship sent to [La Rochelle] for repairs.";
    REQUIRE( f( W.add_unit_on_map(
                 e_unit_type::privateer, { .x = 0, .y = 0 },
                 e_nation::french ) ) == expected );

    Colony& colony = W.add_colony( { .x = 1, .y = 0 } );
    colony.name    = "some colony";
    reason         = e_ship_damaged_reason::colony_abandoned;
    port           = ShipRepairPort::colony{ .id = colony.id };
    expected =
        "[Spanish] [Man-O-War] damaged during colony collapse! "
        "Ship sent to [some colony] for repairs.";
    REQUIRE( f( W.add_unit_on_map(
                 e_unit_type::man_o_war, { .x = 0, .y = 0 },
                 e_nation::spanish ) ) == expected );
  }

  SECTION( "colony_starved" ) {
    reason = e_ship_damaged_reason::colony_starved;
    port   = ShipRepairPort::european_harbor{};
    expected =
        "[French] [Privateer] damaged during colony collapse! "
        "Ship sent to [La Rochelle] for repairs.";
    REQUIRE( f( W.add_unit_on_map(
                 e_unit_type::privateer, { .x = 0, .y = 0 },
                 e_nation::french ) ) == expected );

    Colony& colony = W.add_colony( { .x = 1, .y = 0 } );
    colony.name    = "some colony";
    reason         = e_ship_damaged_reason::colony_starved;
    port           = ShipRepairPort::colony{ .id = colony.id };
    expected =
        "[Spanish] [Man-O-War] damaged during colony collapse! "
        "Ship sent to [some colony] for repairs.";
    REQUIRE( f( W.add_unit_on_map(
                 e_unit_type::man_o_war, { .x = 0, .y = 0 },
                 e_nation::spanish ) ) == expected );
  }
}

TEST_CASE( "[damaged] units_lost_on_ship_message" ) {
  World         W;
  maybe<string> expected;

  auto f = [&]( Unit const& ship ) {
    return units_lost_on_ship_message( ship );
  };

  Unit& privateer =
      W.add_unit_on_map( e_unit_type::privateer,
                         { .x = 0, .y = 0 }, e_nation::french );
  Unit& galleon =
      W.add_unit_on_map( e_unit_type::galleon,
                         { .x = 0, .y = 0 }, e_nation::spanish );
  Unit& caravel =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 0, .y = 0 }, e_nation::french );

  W.add_unit_in_cargo( e_unit_type::free_colonist,
                       privateer.id() );
  W.add_unit_in_cargo( e_unit_type::free_colonist,
                       galleon.id() );
  W.add_unit_in_cargo( e_unit_type::free_colonist,
                       galleon.id() );

  expected = "[One] unit onboard has been lost.";
  REQUIRE( f( privateer ) == expected );

  expected = nothing;
  REQUIRE( f( caravel ) == expected );

  expected = "[Two] units onboard have been lost.";
  REQUIRE( f( galleon ) == expected );
}

TEST_CASE( "[damaged] move_damaged_ship_for_repair" ) {
  World          W;
  ShipRepairPort port;

  auto f = [&]( Unit& ship ) {
    move_damaged_ship_for_repair( W.ss(), W.ts(), ship, port );
  };

  Unit& privateer =
      W.add_unit_on_map( e_unit_type::privateer,
                         { .x = 0, .y = 0 }, e_nation::french );
  Unit& galleon =
      W.add_unit_on_map( e_unit_type::galleon,
                         { .x = 0, .y = 0 }, e_nation::spanish );
  Unit& caravel =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 0, .y = 0 }, e_nation::french );

  privateer.sentry();
  galleon.fortify();
  caravel.start_fortify();

  UnitId const on_privateer =
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           privateer.id() )
          .id();
  UnitId const on_caravel_1 =
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           caravel.id() )
          .id();
  UnitId const on_caravel_2 =
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           caravel.id() )
          .id();

  W.add_commodity_in_cargo( e_commodity::rum, galleon.id(),
                            /*starting_slot=*/3 );
  W.add_commodity_in_cargo( e_commodity::rum, galleon.id() );
  W.add_commodity_in_cargo( e_commodity::rum, privateer.id() );

  // If these don't exist then the references are dangling to
  // begin with, but we'll do this anyway in the hope that it'll
  // help catch additional bugs, even in the face of UB.
  REQUIRE( W.units().exists( privateer.id() ) );
  REQUIRE( W.units().exists( galleon.id() ) );
  REQUIRE( W.units().exists( caravel.id() ) );
  REQUIRE( W.units().exists( on_privateer ) );
  REQUIRE( W.units().exists( on_caravel_1 ) );
  REQUIRE( W.units().exists( on_caravel_2 ) );
  REQUIRE( privateer.orders() == unit_orders::sentry{} );
  REQUIRE( galleon.orders() == unit_orders::fortified{} );
  REQUIRE( caravel.orders() == unit_orders::fortifying{} );
  REQUIRE( privateer.cargo().slots_occupied() == 2 );
  REQUIRE( caravel.cargo().slots_occupied() == 2 );
  REQUIRE( galleon.cargo().slots_occupied() == 2 );
  REQUIRE(
      as_const( W.units() ).ownership_of( privateer.id() ) ==
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );
  REQUIRE( as_const( W.units() ).ownership_of( galleon.id() ) ==
           UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );
  REQUIRE( as_const( W.units() ).ownership_of( caravel.id() ) ==
           UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );

  SECTION( "to european harbor" ) {
    port = ShipRepairPort::european_harbor{};

    f( privateer );
    REQUIRE( W.units().exists( privateer.id() ) );
    REQUIRE( W.units().exists( galleon.id() ) );
    REQUIRE( W.units().exists( caravel.id() ) );
    REQUIRE( !W.units().exists( on_privateer ) );
    REQUIRE( W.units().exists( on_caravel_1 ) );
    REQUIRE( W.units().exists( on_caravel_2 ) );
    REQUIRE( privateer.orders() ==
             unit_orders::damaged{ .turns_until_repair = 8 } );
    REQUIRE( galleon.orders() == unit_orders::fortified{} );
    REQUIRE( caravel.orders() == unit_orders::fortifying{} );
    REQUIRE( privateer.cargo().slots_occupied() == 0 );
    REQUIRE( caravel.cargo().slots_occupied() == 2 );
    REQUIRE( galleon.cargo().slots_occupied() == 2 );
    REQUIRE(
        as_const( W.units() ).ownership_of( privateer.id() ) ==
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{},
                .sailed_from = nothing,
            } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( caravel.id() ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );

    f( galleon );
    REQUIRE( W.units().exists( privateer.id() ) );
    REQUIRE( W.units().exists( galleon.id() ) );
    REQUIRE( W.units().exists( caravel.id() ) );
    REQUIRE( !W.units().exists( on_privateer ) );
    REQUIRE( W.units().exists( on_caravel_1 ) );
    REQUIRE( W.units().exists( on_caravel_2 ) );
    REQUIRE( privateer.orders() ==
             unit_orders::damaged{ .turns_until_repair = 8 } );
    REQUIRE( galleon.orders() ==
             unit_orders::damaged{ .turns_until_repair = 10 } );
    REQUIRE( caravel.orders() == unit_orders::fortifying{} );
    REQUIRE( privateer.cargo().slots_occupied() == 0 );
    REQUIRE( caravel.cargo().slots_occupied() == 2 );
    REQUIRE( galleon.cargo().slots_occupied() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( privateer.id() ) ==
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{},
                .sailed_from = nothing,
            } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{},
                .sailed_from = nothing,
            } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( caravel.id() ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );

    f( caravel );
    REQUIRE( W.units().exists( privateer.id() ) );
    REQUIRE( W.units().exists( galleon.id() ) );
    REQUIRE( W.units().exists( caravel.id() ) );
    REQUIRE( !W.units().exists( on_privateer ) );
    REQUIRE( !W.units().exists( on_caravel_1 ) );
    REQUIRE( !W.units().exists( on_caravel_2 ) );
    REQUIRE( privateer.orders() ==
             unit_orders::damaged{ .turns_until_repair = 8 } );
    REQUIRE( galleon.orders() ==
             unit_orders::damaged{ .turns_until_repair = 10 } );
    REQUIRE( caravel.orders() ==
             unit_orders::damaged{ .turns_until_repair = 2 } );
    REQUIRE( privateer.cargo().slots_occupied() == 0 );
    REQUIRE( caravel.cargo().slots_occupied() == 0 );
    REQUIRE( galleon.cargo().slots_occupied() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( privateer.id() ) ==
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{},
                .sailed_from = nothing,
            } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{},
                .sailed_from = nothing,
            } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( caravel.id() ) ==
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{},
                .sailed_from = nothing,
            } } );
  }

  SECTION( "to colony" ) {
    Colony const& french_colony =
        W.add_colony( { .x = 1, .y = 0 }, e_nation::french );
    Colony const& spanish_colony =
        W.add_colony( { .x = 0, .y = 1 }, e_nation::spanish );

    port = ShipRepairPort::colony{ .id = french_colony.id };
    f( privateer );
    REQUIRE( W.units().exists( privateer.id() ) );
    REQUIRE( W.units().exists( galleon.id() ) );
    REQUIRE( W.units().exists( caravel.id() ) );
    REQUIRE( !W.units().exists( on_privateer ) );
    REQUIRE( W.units().exists( on_caravel_1 ) );
    REQUIRE( W.units().exists( on_caravel_2 ) );
    REQUIRE( privateer.orders() ==
             unit_orders::damaged{ .turns_until_repair = 3 } );
    REQUIRE( galleon.orders() == unit_orders::fortified{} );
    REQUIRE( caravel.orders() == unit_orders::fortifying{} );
    REQUIRE( privateer.cargo().slots_occupied() == 0 );
    REQUIRE( caravel.cargo().slots_occupied() == 2 );
    REQUIRE( galleon.cargo().slots_occupied() == 2 );
    REQUIRE(
        as_const( W.units() ).ownership_of( privateer.id() ) ==
        UnitOwnership::world{ .coord =
                                  french_colony.location } );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );
    REQUIRE(
        as_const( W.units() ).ownership_of( caravel.id() ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );

    port = ShipRepairPort::colony{ .id = spanish_colony.id };
    f( galleon );
    REQUIRE( W.units().exists( privateer.id() ) );
    REQUIRE( W.units().exists( galleon.id() ) );
    REQUIRE( W.units().exists( caravel.id() ) );
    REQUIRE( !W.units().exists( on_privateer ) );
    REQUIRE( W.units().exists( on_caravel_1 ) );
    REQUIRE( W.units().exists( on_caravel_2 ) );
    REQUIRE( privateer.orders() ==
             unit_orders::damaged{ .turns_until_repair = 3 } );
    REQUIRE( galleon.orders() ==
             unit_orders::damaged{ .turns_until_repair = 4 } );
    REQUIRE( caravel.orders() == unit_orders::fortifying{} );
    REQUIRE( privateer.cargo().slots_occupied() == 0 );
    REQUIRE( caravel.cargo().slots_occupied() == 2 );
    REQUIRE( galleon.cargo().slots_occupied() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( privateer.id() ) ==
        UnitOwnership::world{ .coord =
                                  french_colony.location } );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::world{ .coord =
                                  spanish_colony.location } );
    REQUIRE(
        as_const( W.units() ).ownership_of( caravel.id() ) ==
        UnitOwnership::world{ .coord = { .x = 0, .y = 0 } } );

    port = ShipRepairPort::colony{ .id = french_colony.id };
    f( caravel );
    REQUIRE( W.units().exists( privateer.id() ) );
    REQUIRE( W.units().exists( galleon.id() ) );
    REQUIRE( W.units().exists( caravel.id() ) );
    REQUIRE( !W.units().exists( on_privateer ) );
    REQUIRE( !W.units().exists( on_caravel_1 ) );
    REQUIRE( !W.units().exists( on_caravel_2 ) );
    REQUIRE( privateer.orders() ==
             unit_orders::damaged{ .turns_until_repair = 3 } );
    REQUIRE( galleon.orders() ==
             unit_orders::damaged{ .turns_until_repair = 4 } );
    REQUIRE( caravel.orders() == unit_orders::none{} );
    REQUIRE( privateer.cargo().slots_occupied() == 0 );
    REQUIRE( caravel.cargo().slots_occupied() == 0 );
    REQUIRE( galleon.cargo().slots_occupied() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( privateer.id() ) ==
        UnitOwnership::world{ .coord =
                                  french_colony.location } );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::world{ .coord =
                                  spanish_colony.location } );
    REQUIRE(
        as_const( W.units() ).ownership_of( caravel.id() ) ==
        UnitOwnership::world{ .coord =
                                  french_colony.location } );
  }
}

TEST_CASE( "[damaged] ship_damaged_reason" ) {
  e_ship_damaged_reason reason = {};
  string                expected;

  auto f = [&] { return ship_damaged_reason( reason ); };

  reason   = e_ship_damaged_reason::battle;
  expected = "in battle";
  REQUIRE( f() == expected );

  reason   = e_ship_damaged_reason::colony_abandoned;
  expected = "during colony collapse";
  REQUIRE( f() == expected );

  reason   = e_ship_damaged_reason::colony_starved;
  expected = "during colony collapse";
  REQUIRE( f() == expected );
}

TEST_CASE( "[damaged] ship_repair_port_name" ) {
  World          W;
  ShipRepairPort port;
  string         expected;
  e_nation       nation = {};

  auto f = [&] {
    return ship_repair_port_name( W.ss(), nation, port );
  };

  SECTION( "colony" ) {
    Colony& colony = W.add_colony( { .x = 1, .y = 0 } );
    colony.name    = "some colony";
    nation         = colony.nation; // should be irrelevant.
    port           = ShipRepairPort::colony{ .id = colony.id };
    expected       = "some colony";
    REQUIRE( f() == expected );
  }

  SECTION( "harbor" ) {
    nation   = e_nation::french;
    port     = ShipRepairPort::european_harbor{};
    expected = "La Rochelle";
    REQUIRE( f() == expected );

    nation   = e_nation::spanish;
    port     = ShipRepairPort::european_harbor{};
    expected = "Seville";
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
