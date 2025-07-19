/****************************************************************
**raid-effects.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-04-17.
*
* Description: Unit tests for the src/raid-effects.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/raid-effects.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

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
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, L, L, L, //
      L, L, L, L, L, L, //
      _, L, L, L, L, L, //
      L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 6 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[raid] select_brave_attack_colony_effect" ) {
  World W;
  BraveAttackColonyEffect expected;

  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );

  auto f = [&] {
    return select_brave_attack_colony_effect( W.ss(), W.rand(),
                                              colony );
  };

  MockIRand& R = W.rand();

  SECTION( "none" ) {
    expected = BraveAttackColonyEffect::none{};
    R.EXPECT__between_ints( 0, 100 - 1 ).returns( 0 );
    REQUIRE( f() == expected );
    R.EXPECT__between_ints( 0, 100 - 1 ).returns( 11 );
    REQUIRE( f() == expected );
  }

  SECTION( "commodity stolen" ) {
    auto& comms = colony.commodities;
    using C     = e_commodity;

    SECTION( "no comms" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "not enough quantity" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      comms[C::food]   = 4;
      comms[C::silver] = 4;
      comms[C::cloth]  = 4;
      expected         = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "one with just enough quantity" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      comms[C::food]   = 4;
      comms[C::silver] = 5;
      comms[C::cloth]  = 4;
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      R.EXPECT__between_ints( 5, 5 ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::silver,
                           .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with enough quantity" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 6;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 4;
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      R.EXPECT__between_ints( 5, 5 ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::silver,
                           .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 10;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 4;
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      R.EXPECT__between_ints( 5, 5 ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::silver,
                           .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 12;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 4;
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      R.EXPECT__between_ints( 5, 6 ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::silver,
                           .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 12;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 24;
      R.EXPECT__between_ints( 0, 2 - 1 ).returns( 1 );
      R.EXPECT__between_ints( 5, 12 ).returns( 11 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::muskets,
                           .quantity = 11 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      comms[C::cigars]      = 0;
      comms[C::cloth]       = 10;
      comms[C::coats]       = 20;
      comms[C::cotton]      = 30;
      comms[C::food]        = 40;
      comms[C::furs]        = 50;
      comms[C::horses]      = 60;
      comms[C::lumber]      = 70;
      comms[C::muskets]     = 80;
      comms[C::ore]         = 90;
      comms[C::rum]         = 100;
      comms[C::silver]      = 110;
      comms[C::sugar]       = 120;
      comms[C::tobacco]     = 130;
      comms[C::tools]       = 140;
      comms[C::trade_goods] = 150;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      R.EXPECT__between_ints( 0, 15 - 1 ).returns( 9 );
      R.EXPECT__between_ints( 5, 50 ).returns( 50 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::rum,
                           .quantity = 50 } };
      REQUIRE( f() == expected );
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 12 );
      R.EXPECT__between_ints( 0, 15 - 1 ).returns( 9 );
      R.EXPECT__between_ints( 5, 50 ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type     = e_commodity::rum,
                           .quantity = 5 } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "money stolen" ) {
    int& money = W.default_player().money;

    SECTION( "no money" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      money    = 0;
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=2" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      money    = 2;
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=3, res=0" ) {
      money = 3;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 0, 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=3, res=1" ) {
      money = 3;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 0, 1 ).returns( 1 );
      expected =
          BraveAttackColonyEffect::money_stolen{ .quantity = 1 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=10, res=2" ) {
      money = 10;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 0, 2 ).returns( 2 );
      expected =
          BraveAttackColonyEffect::money_stolen{ .quantity = 2 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=10, res=2, with stockade" ) {
      colony.buildings[e_colony_building::stockade] = true;
      money                                         = 10;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=10000, res=1234" ) {
      money = 10000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 300, 2000 ).returns( 1234 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 1234 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=100000, res=12345" ) {
      money = 100000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 3000, 20000 ).returns( 12345 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 12345 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=200000, res=12345" ) {
      money = 200000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 6000, 25000 ).returns( 12345 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 12345 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=200000, res=12345, with fort" ) {
      colony.buildings[e_colony_building::fort] = true;
      money                                     = 200000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=400000, res=12000" ) {
      money = 400000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 12000, 25000 ).returns( 12000 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 12000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=800000, res=25000" ) {
      money = 800000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 24000, 25000 ).returns( 25000 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 25000 };
      REQUIRE( f() == expected );
    }

    // This one yields an upper limit that is naturally right on
    // the boundary of the maximum.
    SECTION( "money=1000000, res=25000" ) {
      money = 1000000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 25000, 25000 ).returns( 25000 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 25000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=1100000, res=20000" ) {
      money = 1100000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      R.EXPECT__between_ints( 25000, 25000 ).returns( 20000 );
      expected = BraveAttackColonyEffect::money_stolen{
        .quantity = 20000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=1100000, res=20000, with fortress" ) {
      colony.buildings[e_colony_building::fortress] = true;
      money                                         = 1100000;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 42 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }
  }

  SECTION( "building destroyed" ) {
    auto& buildings = colony.buildings;
    using B         = e_colony_building;

    SECTION( "attempt=weaver's house" ) {
      buildings[B::weavers_house] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 3 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop" ) {
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 3 );
      expected = BraveAttackColonyEffect::building_destroyed{
        .which = B::weavers_shop };
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop, with stockade" ) {
      buildings[B::stockade]     = true;
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 3 );
      expected = BraveAttackColonyEffect::building_destroyed{
        .which = B::weavers_shop };
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop, with fort" ) {
      buildings[B::fort]         = true;
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop, with fortress" ) {
      buildings[B::fortress]     = true;
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=town hall" ) {
      buildings[B::town_hall] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      // slot = town hall.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 7 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=stockade" ) {
      buildings[B::stockade]  = true;
      buildings[B::town_hall] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      // slot = wall.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 12 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=non existent hammers" ) {
      buildings[B::lumber_mill]     = false;
      buildings[B::carpenters_shop] = false;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 63 );
      // slot = hammers.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 6 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=textile mill" ) {
      buildings[B::textile_mill] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 74 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 3 );
      expected = BraveAttackColonyEffect::building_destroyed{
        .which = B::textile_mill };
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=custom house" ) {
      buildings[B::custom_house] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 74 );
      // slot = custom_house.
      R.EXPECT__between_ints( 0, 16 - 1 ).returns( 15 );
      expected = BraveAttackColonyEffect::building_destroyed{
        .which = B::custom_house };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "ship in port damaged" ) {
    auto add_ship = [&]( e_unit_type type ) -> Unit& {
      return W.add_unit_on_map( type, colony.location,
                                W.default_player_type() );
    };

    SECTION( "no ships" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "one ship" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "one damaged ship" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      Unit& ship    = add_ship( e_unit_type::caravel );
      ship.orders() = unit_orders::damaged{};
      expected      = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "two ships" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      add_ship( e_unit_type::caravel );
      UnitId const ship_id2 =
          add_ship( e_unit_type::galleon ).id();
      R.EXPECT__between_ints( 0, 2 - 1 ).returns( 1 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id2,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      UnitId const ship_id1 =
          add_ship( e_unit_type::caravel ).id();
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      R.EXPECT__between_ints( 0, 3 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id1,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, stockade" ) {
      colony.buildings[e_colony_building::stockade] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      UnitId const ship_id1 =
          add_ship( e_unit_type::caravel ).id();
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      R.EXPECT__between_ints( 0, 3 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id1,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, fort" ) {
      colony.buildings[e_colony_building::fort] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      UnitId const ship_id1 =
          add_ship( e_unit_type::caravel ).id();
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      R.EXPECT__between_ints( 0, 3 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id1,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, fortress" ) {
      colony.buildings[e_colony_building::fortress] = true;
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      add_ship( e_unit_type::caravel );
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, one damaged" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      add_ship( e_unit_type::caravel );
      Unit& galleon = add_ship( e_unit_type::galleon );
      UnitId const ship_id3 =
          add_ship( e_unit_type::merchantman ).id();
      galleon.orders() = unit_orders::damaged{};
      R.EXPECT__between_ints( 0, 2 - 1 ).returns( 1 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id3,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, all damaged" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      Unit& caravel     = add_ship( e_unit_type::caravel );
      Unit& galleon     = add_ship( e_unit_type::galleon );
      Unit& merchantman = add_ship( e_unit_type::merchantman );
      caravel.orders()  = unit_orders::damaged{};
      galleon.orders()  = unit_orders::damaged{};
      merchantman.orders() = unit_orders::damaged{};
      expected             = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "one ship, second colony no drydock" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      W.add_colony( { .x = 0, .y = 1 } );
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id,
        .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "one ship, second colony with drydock" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      Colony& colony2 = W.add_colony( { .x = 0, .y = 1 } );
      colony2.buildings[e_colony_building::drydock] = true;
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id,
        .sent_to = ShipRepairPort::colony{ .id = colony2.id } };
      REQUIRE( f() == expected );
    }

    SECTION( "one ship, second colony with shipyard" ) {
      R.EXPECT__between_ints( 0, 100 - 1 ).returns( 75 );
      Colony& colony2 = W.add_colony( { .x = 0, .y = 1 } );
      colony2.buildings[e_colony_building::shipyard] = true;
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1 - 1 ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
        .which   = ship_id,
        .sent_to = ShipRepairPort::colony{ .id = colony2.id } };
      REQUIRE( f() == expected );
    }
  }
}

TEST_CASE( "[raid] perform_brave_attack_colony_effect" ) {
  World W;

  Colony& colony        = W.add_colony( { .x = 1, .y = 1 } );
  Colony& colony_repair = W.add_colony( { .x = 1, .y = 2 } );
  BraveAttackColonyEffect effect;

  colony.commodities[e_commodity::sugar] = 50;
  colony.commodities[e_commodity::coats] = 70;

  colony_repair.buildings[e_colony_building::drydock] = true;

  Player& player = W.default_player();
  player.money   = 1234;

  colony.buildings[e_colony_building::blacksmiths_shop] = true;
  colony.buildings[e_colony_building::iron_works]       = true;
  colony.buildings[e_colony_building::newspaper]        = true;

  Colony old_colony              = colony;
  Colony const old_colony_repair = colony_repair;

  Unit const& ship1 =
      W.add_unit_on_map( e_unit_type::frigate, colony.location );
  Unit const& ship2 =
      W.add_unit_on_map( e_unit_type::frigate, colony.location );

  Tribe& tribe = W.add_tribe( e_tribe::tupi );

  auto f = [&] {
    perform_brave_attack_colony_effect( W.ss(), W.ts(), colony,
                                        tribe, effect );
  };

  // No effect.
  effect = BraveAttackColonyEffect::none{};
  f();
  REQUIRE( colony == old_colony );
  REQUIRE( player.money == 1234 );
  REQUIRE( ship1.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( ship2.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  // Commodity stolen.
  effect = BraveAttackColonyEffect::commodity_stolen{
    .what = Commodity{ .type     = e_commodity::coats,
                       .quantity = 20 } };
  f();
  REQUIRE( colony.commodities[e_commodity::coats] == 50 );
  old_colony.commodities[e_commodity::coats] = 50;
  REQUIRE( colony == old_colony );
  REQUIRE( player.money == 1234 );
  REQUIRE( ship1.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( ship2.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  // Money stolen.
  effect =
      BraveAttackColonyEffect::money_stolen{ .quantity = 234 };
  f();
  REQUIRE( colony == old_colony );
  REQUIRE( player.money == 1000 );
  REQUIRE( ship1.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( ship2.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  // Building destroyed.
  effect = BraveAttackColonyEffect::building_destroyed{
    .which = e_colony_building::blacksmiths_shop };
  f();
  REQUIRE(
      colony.buildings[e_colony_building::blacksmiths_shop] ==
      false );
  old_colony.buildings[e_colony_building::blacksmiths_shop] =
      false;
  REQUIRE( colony == old_colony );
  effect = BraveAttackColonyEffect::building_destroyed{
    .which = e_colony_building::iron_works };
  f();
  REQUIRE( colony.buildings[e_colony_building::iron_works] ==
           false );
  old_colony.buildings[e_colony_building::iron_works] = false;
  REQUIRE( colony == old_colony );
  REQUIRE(
      colony.buildings[e_colony_building::blacksmiths_house] ==
      true );
  REQUIRE( colony.buildings[e_colony_building::newspaper] ==
           true );
  REQUIRE( player.money == 1000 );
  REQUIRE( ship1.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( ship2.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  // Ship in port damaged.
  effect = BraveAttackColonyEffect::ship_in_port_damaged{
    .which   = ship1.id(),
    .sent_to = ShipRepairPort::european_harbor{} };
  f();
  REQUIRE( colony == old_colony );
  REQUIRE( player.money == 1000 );
  REQUIRE( ship1.orders() ==
           unit_orders::damaged{ .turns_until_repair = 12 } );
  REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::in_port{},
             .sailed_from = nothing } );
  REQUIRE( ship2.orders() == unit_orders::none{} );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 1 } } );
  effect = BraveAttackColonyEffect::ship_in_port_damaged{
    .which = ship2.id(),
    .sent_to =
        ShipRepairPort::colony{ .id = colony_repair.id } };
  f();
  REQUIRE( colony == old_colony );
  REQUIRE( colony_repair == old_colony_repair );
  REQUIRE( player.money == 1000 );
  REQUIRE( ship1.orders() ==
           unit_orders::damaged{ .turns_until_repair = 12 } );
  REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::in_port{},
             .sailed_from = nothing } );
  REQUIRE( ship2.orders() ==
           unit_orders::damaged{ .turns_until_repair = 5 } );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 2 } } );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );
}

TEST_CASE(
    "[raid] perform_brave_attack_colony_effect (muskets "
    "stolen)" ) {
  World W;

  BraveAttackColonyEffect effect =
      BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type = e_commodity::muskets } };
  int& quantity =
      effect.get<BraveAttackColonyEffect::commodity_stolen>()
          .what.quantity;

  Tribe& tribe = W.add_tribe( e_tribe::tupi );

  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );
  colony.commodities[e_commodity::sugar]   = 50;
  colony.commodities[e_commodity::coats]   = 70;
  colony.commodities[e_commodity::muskets] = 300;
  colony.commodities[e_commodity::horses]  = 300;

  Colony old_colony = colony;

  auto f = [&] {
    perform_brave_attack_colony_effect( W.ss(), W.ts(), colony,
                                        tribe, effect );
  };

  SECTION( "stole 1 muskets" ) {
    quantity = 1;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 299 );
    old_colony.commodities[e_commodity::muskets] = 299;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 2 muskets" ) {
    quantity = 2;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 298 );
    old_colony.commodities[e_commodity::muskets] = 298;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 24 muskets" ) {
    quantity = 24;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 276 );
    old_colony.commodities[e_commodity::muskets] = 276;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 25 muskets" ) {
    quantity = 25;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 275 );
    old_colony.commodities[e_commodity::muskets] = 275;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 49 muskets" ) {
    quantity = 49;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 251 );
    old_colony.commodities[e_commodity::muskets] = 251;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 50 muskets" ) {
    quantity = 50;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 250 );
    old_colony.commodities[e_commodity::muskets] = 250;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 2 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 51 muskets" ) {
    quantity = 51;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 249 );
    old_colony.commodities[e_commodity::muskets] = 249;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 2 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "stole 100 muskets" ) {
    quantity             = 100;
    tribe.muskets        = 5;
    tribe.horse_herds    = 5;
    tribe.horse_breeding = 5;
    f();
    REQUIRE( colony.commodities[e_commodity::muskets] == 200 );
    old_colony.commodities[e_commodity::muskets] = 200;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 5 + 2 );
    REQUIRE( tribe.horse_herds == 5 + 0 );
    REQUIRE( tribe.horse_breeding == 5 + 0 );
  }
}

TEST_CASE(
    "[raid] perform_brave_attack_colony_effect (horses "
    "stolen)" ) {
  World W;

  BraveAttackColonyEffect effect =
      BraveAttackColonyEffect::commodity_stolen{
        .what = Commodity{ .type = e_commodity::horses } };
  int& quantity =
      effect.get<BraveAttackColonyEffect::commodity_stolen>()
          .what.quantity;

  Tribe& tribe = W.add_tribe( e_tribe::tupi );

  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );
  colony.commodities[e_commodity::sugar]   = 50;
  colony.commodities[e_commodity::coats]   = 70;
  colony.commodities[e_commodity::muskets] = 300;
  colony.commodities[e_commodity::horses]  = 300;

  Colony old_colony = colony;

  auto f = [&] {
    perform_brave_attack_colony_effect( W.ss(), W.ts(), colony,
                                        tribe, effect );
  };

  SECTION( "stole 1 horses" ) {
    quantity = 1;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 299 );
    old_colony.commodities[e_commodity::horses] = 299;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 2 horses" ) {
    quantity = 2;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 298 );
    old_colony.commodities[e_commodity::horses] = 298;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 24 horses" ) {
    quantity = 24;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 276 );
    old_colony.commodities[e_commodity::horses] = 276;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 25 horses" ) {
    quantity = 25;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 275 );
    old_colony.commodities[e_commodity::horses] = 275;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 49 horses" ) {
    quantity = 49;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 251 );
    old_colony.commodities[e_commodity::horses] = 251;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 50 horses" ) {
    quantity = 50;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 250 );
    old_colony.commodities[e_commodity::horses] = 250;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 51 horses" ) {
    quantity = 51;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 249 );
    old_colony.commodities[e_commodity::horses] = 249;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 25 );
  }

  SECTION( "stole 100 horses" ) {
    quantity             = 100;
    tribe.muskets        = 5;
    tribe.horse_herds    = 5;
    tribe.horse_breeding = 5;
    f();
    REQUIRE( colony.commodities[e_commodity::horses] == 200 );
    old_colony.commodities[e_commodity::horses] = 200;
    REQUIRE( colony == old_colony );
    REQUIRE( tribe.muskets == 5 + 0 );
    REQUIRE( tribe.horse_herds == 5 + 1 );
    REQUIRE( tribe.horse_breeding == 5 + 25 );
  }
}

TEST_CASE( "[raid] display_brave_attack_colony_effect_msg" ) {
  World W;

  MockIAgent& agent = W.agent( W.default_player_type() );
  Colony& colony    = W.add_colony( { .x = 1, .y = 1 } );
  colony.name       = "my colony";
  Unit const& ship =
      W.add_unit_on_map( e_unit_type::frigate, colony.location );
  BraveAttackColonyEffect effect;

  auto f = [&] {
    wait<> const w = display_brave_attack_colony_effect_msg(
        W.ss(), agent, colony, effect, e_tribe::inca );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  // No effect.
  effect = BraveAttackColonyEffect::none{};
  f();

  // Commodity stolen.
  effect = BraveAttackColonyEffect::commodity_stolen{
    .what = Commodity{ .type     = e_commodity::coats,
                       .quantity = 20 } };
  agent.EXPECT__message_box(
      "[Inca] looting parties have stolen [20] tons of "
      "[coats] from [my colony]!" );
  f();

  // Money stolen.
  effect =
      BraveAttackColonyEffect::money_stolen{ .quantity = 234 };
  agent.EXPECT__message_box(
      "[Inca] looting parties have stolen [234\x7f] from "
      "the treasury!" );
  f();

  // Building destroyed.
  effect = BraveAttackColonyEffect::building_destroyed{
    .which = e_colony_building::blacksmiths_shop };
  agent.EXPECT__message_box(
      "[Inca] raiding parties have destroyed the "
      "[Blacksmith's Shop] in [my colony]!" );
  f();

  // Ship in port damaged.
  effect = BraveAttackColonyEffect::ship_in_port_damaged{
    .which   = ship.id(),
    .sent_to = ShipRepairPort::european_harbor{} };
  agent.EXPECT__message_box(
      "[Dutch] [Frigate] damaged in battle! Ship sent to "
      "[Amsterdam] for repairs." );
  f();
}

} // namespace
} // namespace rn
