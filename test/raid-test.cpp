/****************************************************************
**raid.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-04-17.
*
* Description: Unit tests for the src/raid.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/raid.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"

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
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
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
  }
}

TEST_CASE( "[raid] perform_brave_attack_colony_effect" ) {
  World W;
}

TEST_CASE( "[raid] display_brave_attack_colony_effect_msg" ) {
  World W;
}

} // namespace
} // namespace rn
