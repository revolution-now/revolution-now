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
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/unit-composer.hpp"
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
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
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
  World                   W;
  BraveAttackColonyEffect expected;

  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );

  auto f = [&] {
    return select_brave_attack_colony_effect( W.ss(), W.rand(),
                                              colony );
  };

  MockIRand& R = W.rand();
  using enum e_interval;

  SECTION( "none" ) {
    expected = BraveAttackColonyEffect::none{};
    R.EXPECT__between_ints( 0, 100, half_open ).returns( 0 );
    REQUIRE( f() == expected );
    R.EXPECT__between_ints( 0, 100, half_open ).returns( 11 );
    REQUIRE( f() == expected );
  }

  SECTION( "commodity stolen" ) {
    auto& comms = colony.commodities;
    using C     = e_commodity;

    SECTION( "no comms" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "not enough quantity" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      comms[C::food]   = 4;
      comms[C::silver] = 4;
      comms[C::cloth]  = 4;
      expected         = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "one with just enough quantity" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      comms[C::food]   = 4;
      comms[C::silver] = 5;
      comms[C::cloth]  = 4;
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      R.EXPECT__between_ints( 5, 5, closed ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
          .what = Commodity{ .type     = e_commodity::silver,
                             .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with enough quantity" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 6;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 4;
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      R.EXPECT__between_ints( 5, 5, closed ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
          .what = Commodity{ .type     = e_commodity::silver,
                             .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 10;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 4;
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      R.EXPECT__between_ints( 5, 5, closed ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
          .what = Commodity{ .type     = e_commodity::silver,
                             .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 12;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 4;
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      R.EXPECT__between_ints( 5, 6, closed ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
          .what = Commodity{ .type     = e_commodity::silver,
                             .quantity = 5 } };
      REQUIRE( f() == expected );
    }

    SECTION( "one with 10 of one commodity" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      comms[C::food]    = 4;
      comms[C::silver]  = 12;
      comms[C::cloth]   = 4;
      comms[C::muskets] = 24;
      R.EXPECT__between_ints( 0, 2, half_open ).returns( 1 );
      R.EXPECT__between_ints( 5, 12, closed ).returns( 11 );
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
      comms[C::fur]         = 50;
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
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      R.EXPECT__between_ints( 0, 15, half_open ).returns( 9 );
      R.EXPECT__between_ints( 5, 50, closed ).returns( 50 );
      expected = BraveAttackColonyEffect::commodity_stolen{
          .what = Commodity{ .type     = e_commodity::rum,
                             .quantity = 50 } };
      REQUIRE( f() == expected );
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 12 );
      R.EXPECT__between_ints( 0, 15, half_open ).returns( 9 );
      R.EXPECT__between_ints( 5, 50, closed ).returns( 5 );
      expected = BraveAttackColonyEffect::commodity_stolen{
          .what = Commodity{ .type     = e_commodity::rum,
                             .quantity = 5 } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "money stolen" ) {
    int& money = W.default_player().money;

    SECTION( "no money" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      money    = 0;
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=2" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      money    = 2;
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=3, res=0" ) {
      money = 3;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 0, 1, closed ).returns( 0 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=3, res=1" ) {
      money = 3;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 0, 1, closed ).returns( 1 );
      expected =
          BraveAttackColonyEffect::money_stolen{ .quantity = 1 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=10, res=2" ) {
      money = 10;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 0, 2, closed ).returns( 2 );
      expected =
          BraveAttackColonyEffect::money_stolen{ .quantity = 2 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=10, res=2, with stockade" ) {
      colony.buildings[e_colony_building::stockade] = true;
      money                                         = 10;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=10000, res=1234" ) {
      money = 10000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 300, 2000, closed )
          .returns( 1234 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 1234 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=100000, res=12345" ) {
      money = 100000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 3000, 20000, closed )
          .returns( 12345 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 12345 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=200000, res=12345" ) {
      money = 200000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 6000, 25000, closed )
          .returns( 12345 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 12345 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=200000, res=12345, with fort" ) {
      colony.buildings[e_colony_building::fort] = true;
      money                                     = 200000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=400000, res=12000" ) {
      money = 400000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 12000, 25000, closed )
          .returns( 12000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 12000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=800000, res=25000" ) {
      money = 800000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 24000, 25000, closed )
          .returns( 25000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 25000 };
      REQUIRE( f() == expected );
    }

    // This one yields an upper limit that is naturally right on
    // the boundary of the maximum.
    SECTION( "money=1000000, res=25000" ) {
      money = 1000000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 25000, 25000, closed )
          .returns( 25000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 25000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=1100000, res=20000" ) {
      money = 1100000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      R.EXPECT__between_ints( 25000, 25000, closed )
          .returns( 20000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 20000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=1100000, res=20000, with fortress" ) {
      colony.buildings[e_colony_building::fortress] = true;
      money                                         = 1100000;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }
  }

  SECTION( "building destroyed" ) {
    auto& buildings = colony.buildings;
    using B         = e_colony_building;

    SECTION( "attempt=weaver's house" ) {
      buildings[B::weavers_house] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 3 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop" ) {
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 3 );
      expected = BraveAttackColonyEffect::building_destroyed{
          .which = B::weavers_shop };
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop, with stockade" ) {
      buildings[B::stockade]     = true;
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 3 );
      expected = BraveAttackColonyEffect::building_destroyed{
          .which = B::weavers_shop };
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop, with fort" ) {
      buildings[B::fort]         = true;
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=weaver's shop, with fortress" ) {
      buildings[B::fortress]     = true;
      buildings[B::weavers_shop] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=town hall" ) {
      buildings[B::town_hall] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      // slot = town hall.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 7 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=stockade" ) {
      buildings[B::stockade]  = true;
      buildings[B::town_hall] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      // slot = wall.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 12 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=non existent hammers" ) {
      buildings[B::lumber_mill]     = false;
      buildings[B::carpenters_shop] = false;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 63 );
      // slot = hammers.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 6 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=textile mill" ) {
      buildings[B::textile_mill] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 74 );
      // slot = cloth.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 3 );
      expected = BraveAttackColonyEffect::building_destroyed{
          .which = B::textile_mill };
      REQUIRE( f() == expected );
    }

    SECTION( "attempt=custom house" ) {
      buildings[B::custom_house] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 74 );
      // slot = custom_house.
      R.EXPECT__between_ints( 0, 16, half_open ).returns( 15 );
      expected = BraveAttackColonyEffect::building_destroyed{
          .which = B::custom_house };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "ship in port damaged" ) {
    auto add_ship = [&]( e_unit_type type ) -> Unit& {
      return W.add_unit_on_map( type, colony.location,
                                W.default_nation() );
    };

    SECTION( "no ships" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "one ship" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "one damaged ship" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      Unit& ship    = add_ship( e_unit_type::caravel );
      ship.orders() = unit_orders::damaged{};
      expected      = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "two ships" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      add_ship( e_unit_type::caravel );
      UnitId const ship_id2 =
          add_ship( e_unit_type::galleon ).id();
      R.EXPECT__between_ints( 0, 2, half_open ).returns( 1 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id2,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      UnitId const ship_id1 =
          add_ship( e_unit_type::caravel ).id();
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      R.EXPECT__between_ints( 0, 3, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id1,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, stockade" ) {
      colony.buildings[e_colony_building::stockade] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      UnitId const ship_id1 =
          add_ship( e_unit_type::caravel ).id();
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      R.EXPECT__between_ints( 0, 3, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id1,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, fort" ) {
      colony.buildings[e_colony_building::fort] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      UnitId const ship_id1 =
          add_ship( e_unit_type::caravel ).id();
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      R.EXPECT__between_ints( 0, 3, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id1,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, fortress" ) {
      colony.buildings[e_colony_building::fortress] = true;
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      add_ship( e_unit_type::caravel );
      add_ship( e_unit_type::galleon );
      add_ship( e_unit_type::merchantman );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, one damaged" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      add_ship( e_unit_type::caravel );
      Unit&        galleon = add_ship( e_unit_type::galleon );
      UnitId const ship_id3 =
          add_ship( e_unit_type::merchantman ).id();
      galleon.orders() = unit_orders::damaged{};
      R.EXPECT__between_ints( 0, 2, half_open ).returns( 1 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id3,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "three ships, all damaged" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
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
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      W.add_colony( { .x = 0, .y = 1 } );
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which   = ship_id,
          .sent_to = ShipRepairPort::european_harbor{} };
      REQUIRE( f() == expected );
    }

    SECTION( "one ship, second colony with drydock" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      Colony& colony2 = W.add_colony( { .x = 0, .y = 1 } );
      colony2.buildings[e_colony_building::drydock] = true;
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which = ship_id,
          .sent_to =
              ShipRepairPort::colony{ .id = colony2.id } };
      REQUIRE( f() == expected );
    }

    SECTION( "one ship, second colony with shipyard" ) {
      R.EXPECT__between_ints( 0, 100, half_open ).returns( 75 );
      Colony& colony2 = W.add_colony( { .x = 0, .y = 1 } );
      colony2.buildings[e_colony_building::shipyard] = true;
      UnitId const ship_id =
          add_ship( e_unit_type::caravel ).id();
      R.EXPECT__between_ints( 0, 1, half_open ).returns( 0 );
      expected = BraveAttackColonyEffect::ship_in_port_damaged{
          .which = ship_id,
          .sent_to =
              ShipRepairPort::colony{ .id = colony2.id } };
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

  Colony       old_colony        = colony;
  Colony const old_colony_repair = colony_repair;

  Unit const& ship1 =
      W.add_unit_on_map( e_unit_type::frigate, colony.location );
  Unit const& ship2 =
      W.add_unit_on_map( e_unit_type::frigate, colony.location );

  auto f = [&] {
    perform_brave_attack_colony_effect( W.ss(), W.ts(), colony,
                                        effect );
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
               .st = UnitHarborViewState{
                   .port_status = PortStatus::in_port{},
                   .sailed_from = nothing } } );
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
               .st = UnitHarborViewState{
                   .port_status = PortStatus::in_port{},
                   .sailed_from = nothing } } );
  REQUIRE( ship2.orders() ==
           unit_orders::damaged{ .turns_until_repair = 5 } );
  REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
           UnitOwnership::world{ .coord = { .x = 1, .y = 2 } } );
}

TEST_CASE( "[raid] display_brave_attack_colony_effect_msg" ) {
  World W;

  MockIEuroMind& mind   = W.euro_mind( W.default_nation() );
  Colony&        colony = W.add_colony( { .x = 1, .y = 1 } );
  colony.name           = "my colony";
  Unit const& ship =
      W.add_unit_on_map( e_unit_type::frigate, colony.location );
  BraveAttackColonyEffect effect;

  auto f = [&] {
    wait<> const w = display_brave_attack_colony_effect_msg(
        W.ss(), mind, colony, effect, e_tribe::inca );
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
  mind.EXPECT__message_box(
      "[Inca] looting parties have stolen [20] tons of "
      "[coats] from [my colony]!" );
  f();

  // Money stolen.
  effect =
      BraveAttackColonyEffect::money_stolen{ .quantity = 234 };
  mind.EXPECT__message_box(
      "[Inca] looting parties have stolen [234\x7f] from "
      "the treasury!" );
  f();

  // Building destroyed.
  effect = BraveAttackColonyEffect::building_destroyed{
      .which = e_colony_building::blacksmiths_shop };
  mind.EXPECT__message_box(
      "[Inca] raiding parties have destroyed the "
      "[Blacksmith's Shop] in [my colony]!" );
  f();

  // Ship in port damaged.
  effect = BraveAttackColonyEffect::ship_in_port_damaged{
      .which   = ship.id(),
      .sent_to = ShipRepairPort::european_harbor{} };
  mind.EXPECT__message_box(
      "[Dutch] [Frigate] damaged in battle! Ship sent to "
      "[Amsterdam] for repairs." );
  f();
}

} // namespace
} // namespace rn
