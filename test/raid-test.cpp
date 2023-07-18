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
  }

  SECTION( "money stolen" ) {
    R.EXPECT__between_ints( 0, 100, half_open ).returns( 42 );
    int& money = W.default_player().money;

    SECTION( "no money" ) {
      money    = 0;
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=2" ) {
      money    = 2;
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=3, res=0" ) {
      money = 3;
      R.EXPECT__between_ints( 0, 1, closed ).returns( 0 );
      expected = BraveAttackColonyEffect::none{};
      REQUIRE( f() == expected );
    }

    SECTION( "money=3, res=1" ) {
      money = 3;
      R.EXPECT__between_ints( 0, 1, closed ).returns( 1 );
      expected =
          BraveAttackColonyEffect::money_stolen{ .quantity = 1 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=10, res=2" ) {
      money = 10;
      R.EXPECT__between_ints( 0, 2, closed ).returns( 2 );
      expected =
          BraveAttackColonyEffect::money_stolen{ .quantity = 2 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=10000, res=1234" ) {
      money = 10000;
      R.EXPECT__between_ints( 300, 2000, closed )
          .returns( 1234 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 1234 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=100000, res=12345" ) {
      money = 100000;
      R.EXPECT__between_ints( 3000, 20000, closed )
          .returns( 12345 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 12345 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=200000, res=12345" ) {
      money = 200000;
      R.EXPECT__between_ints( 6000, 25000, closed )
          .returns( 12345 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 12345 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=400000, res=12000" ) {
      money = 400000;
      R.EXPECT__between_ints( 12000, 25000, closed )
          .returns( 12000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 12000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=800000, res=25000" ) {
      money = 800000;
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
      R.EXPECT__between_ints( 25000, 25000, closed )
          .returns( 25000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 25000 };
      REQUIRE( f() == expected );
    }

    SECTION( "money=1100000, res=20000" ) {
      money = 1100000;
      R.EXPECT__between_ints( 25000, 25000, closed )
          .returns( 20000 );
      expected = BraveAttackColonyEffect::money_stolen{
          .quantity = 20000 };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "building destroyed" ) {
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
