/****************************************************************
**tribe-arms-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-01-28.
*
* Description: Unit tests for the tribe-arms module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/tribe-arms.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/tribe.rds.hpp"

// gfx
#include "src/gfx/iter.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::Approx;

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
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }

  void create_n_dwellings( e_tribe tribe_type, int n ) {
    gfx::rect_iterator const ri(
        terrain().world_rect_tiles().to_gfx() );
    for( Coord const coord : ri ) {
      if( n-- <= 0 ) return;
      add_dwelling( coord, tribe_type );
    }
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[tribe-arms] retain_muskets_from_destroyed_brave" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::sioux );

  auto f = [&] { retain_muskets_from_destroyed_brave( tribe ); };

  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 1 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 2 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 3 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );
}

TEST_CASE( "[tribe-arms] retain_horses_from_destroyed_brave" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::sioux );

  auto f = [&] {
    retain_horses_from_destroyed_brave( W.ss(), tribe );
  };

  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 25 );

  f();
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 50 );

  f();
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 50 );
}

TEST_CASE( "[tribe-arms] gain_horses_from_winning_combat" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::sioux );

  auto f = [&] { gain_horses_from_winning_combat( tribe ); };

  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 1 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 2 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 3 );
  REQUIRE( tribe.horse_breeding == 0 );
}

TEST_CASE( "[tribe-arms] acquire_muskets_from_colony_raid" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::sioux );

  auto f = [&]( int q ) {
    acquire_muskets_from_colony_raid( tribe, q );
  };

  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 0 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 1 );
  REQUIRE( tribe.muskets == 1 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 24 );
  REQUIRE( tribe.muskets == 2 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 25 );
  REQUIRE( tribe.muskets == 3 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 49 );
  REQUIRE( tribe.muskets == 4 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 50 );
  REQUIRE( tribe.muskets == 6 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 100 );
  REQUIRE( tribe.muskets == 8 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );
}

TEST_CASE( "[tribe-arms] acquire_horses_from_colony_raid" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::cherokee );

  auto f = [&]( int q ) {
    acquire_horses_from_colony_raid( W.ss(), tribe, q );
  };

  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 0 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f( 1 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 1 );
  REQUIRE( tribe.horse_breeding == 25 );

  f( 10 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 2 );
  REQUIRE( tribe.horse_breeding == 50 );

  f( 25 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 3 );
  REQUIRE( tribe.horse_breeding == 54 );

  f( 49 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 4 );
  REQUIRE( tribe.horse_breeding == 54 );

  f( 50 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 5 );
  REQUIRE( tribe.horse_breeding == 54 );

  f( 300000 );
  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 6 );
  REQUIRE( tribe.horse_breeding == 54 );
}

TEST_CASE(
    "[tribe-arms] retain_horses_from_destroyed_brave max "
    "value" ) {
  World  W;
  Tribe* tribe = nullptr;

  auto f = [&] {
    BASE_CHECK( tribe != nullptr );
    retain_horses_from_destroyed_brave( W.ss(), *tribe );
  };

  // semi_nomadic: { N=0, M=6,  A=0 }
  SECTION( "semi-nomadic" ) {
    tribe = &W.add_tribe( e_tribe::sioux );

    SECTION( "dwellings=0" ) { // max = 50
      W.create_n_dwellings( tribe->type, 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 50 );
    }

    SECTION( "dwellings=1" ) { // max = 56
      W.create_n_dwellings( tribe->type, 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 56 );
    }

    SECTION( "dwellings=10" ) { // max = 110
      W.create_n_dwellings( tribe->type, 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 110 );
    }
  }

  // agrarian: { N=4, M=10, A=0 }
  SECTION( "agrarian" ) {
    tribe = &W.add_tribe( e_tribe::cherokee );

    SECTION( "dwellings=0" ) { // max = 54
      W.create_n_dwellings( tribe->type, 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 54 );
    }

    SECTION( "dwellings=1" ) { // max = 64
      W.create_n_dwellings( tribe->type, 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 64 );
    }

    SECTION( "dwellings=10" ) { // max = 154
      W.create_n_dwellings( tribe->type, 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 154 );
    }
  }

  // advanced: { N=6, M=14, A=0 }
  SECTION( "aztec" ) {
    tribe = &W.add_tribe( e_tribe::aztec );

    SECTION( "dwellings=0" ) { // max = 56
      W.create_n_dwellings( tribe->type, 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 56 );
    }

    SECTION( "dwellings=1" ) { // max = 70
      W.create_n_dwellings( tribe->type, 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 70 );
    }

    SECTION( "dwellings=10" ) { // max = 196
      W.create_n_dwellings( tribe->type, 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 196 );
    }
  }

  // civilized: { N=8, M=18, A=0 }
  SECTION( "inca" ) {
    tribe = &W.add_tribe( e_tribe::inca );

    SECTION( "dwellings=0" ) { // max = 58
      W.create_n_dwellings( tribe->type, 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 58 );
    }

    SECTION( "dwellings=1" ) { // max = 76
      W.create_n_dwellings( tribe->type, 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 76 );
    }

    SECTION( "dwellings=10" ) { // max = 238
      W.create_n_dwellings( tribe->type, 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 238 );
    }
  }

  REQUIRE( tribe->horse_herds == 0 );
}

TEST_CASE( "[tribe-arms] select_brave_spawn" ) {
  World         W;
  Tribe&        tribe = W.add_tribe( e_tribe::cherokee );
  EquippedBrave expected;

  auto f = [&] {
    return select_brave_equip( W.ss().as_const, W.rand(),
                               as_const( tribe ) );
  };

  W.settings().difficulty = e_difficulty::governor;

  SECTION( "muskets=0,horse_breeding=0" ) {
    tribe.muskets        = 0;
    tribe.horse_breeding = 0;

    expected = { .type          = e_native_unit_type::brave,
                 .muskets_delta = 0,
                 .horse_breeding_delta = 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=0,horse_breeding=24" ) {
    tribe.muskets        = 0;
    tribe.horse_breeding = 24;

    expected = { .type          = e_native_unit_type::brave,
                 .muskets_delta = 0,
                 .horse_breeding_delta = 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=5,horse_breeding=0, muskets depleted" ) {
    tribe.muskets        = 5;
    tribe.horse_breeding = 0;

    W.rand().EXPECT__bernoulli( .25 ).returns( true );
    expected = { .type = e_native_unit_type::armed_brave,
                 .muskets_delta        = -1,
                 .horse_breeding_delta = 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=5,horse_breeding=0, muskets not depleted" ) {
    tribe.muskets        = 5;
    tribe.horse_breeding = 0;

    W.rand().EXPECT__bernoulli( .25 ).returns( false );
    expected = { .type = e_native_unit_type::armed_brave,
                 .muskets_delta        = 0,
                 .horse_breeding_delta = 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=0,horse_breeding=25" ) {
    tribe.muskets        = 0;
    tribe.horse_breeding = 25;

    expected = { .type = e_native_unit_type::mounted_brave,
                 .muskets_delta        = 0,
                 .horse_breeding_delta = -25 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=0,horse_breeding=100" ) {
    tribe.muskets        = 0;
    tribe.horse_breeding = 100;

    expected = { .type = e_native_unit_type::mounted_brave,
                 .muskets_delta        = 0,
                 .horse_breeding_delta = -25 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=1,horse_breeding=26, muskets depleted" ) {
    tribe.muskets        = 1;
    tribe.horse_breeding = 26;

    W.rand().EXPECT__bernoulli( .25 ).returns( true );
    expected = { .type = e_native_unit_type::mounted_warrior,
                 .muskets_delta        = -1,
                 .horse_breeding_delta = -25 };
    REQUIRE( f() == expected );
  }

  SECTION(
      "muskets=4,horse_breeding=100, muskets not depleeted" ) {
    tribe.muskets        = 4;
    tribe.horse_breeding = 100;

    W.rand().EXPECT__bernoulli( .25 ).returns( false );
    expected = { .type = e_native_unit_type::mounted_warrior,
                 .muskets_delta        = 0,
                 .horse_breeding_delta = -25 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=5,horse_breeding=0,discoverer,depleted" ) {
    W.settings().difficulty = e_difficulty::discoverer;
    tribe.muskets           = 5;
    tribe.horse_breeding    = 0;

    W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
    expected = { .type = e_native_unit_type::armed_brave,
                 .muskets_delta        = -1,
                 .horse_breeding_delta = 0 };
    REQUIRE( f() == expected );
  }

  SECTION(
      "muskets=5,horse_breeding=0,conquistador,not depleted" ) {
    W.settings().difficulty = e_difficulty::conquistador;
    tribe.muskets           = 5;
    tribe.horse_breeding    = 0;

    W.rand()
        .EXPECT__bernoulli( Approx( .333333, .000001 ) )
        .returns( false );
    expected = { .type = e_native_unit_type::armed_brave,
                 .muskets_delta        = 0,
                 .horse_breeding_delta = 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "muskets=5,horse_breeding=25,viceroy,depleted" ) {
    W.settings().difficulty = e_difficulty::viceroy;
    tribe.muskets           = 5;
    tribe.horse_breeding    = 25;

    W.rand().EXPECT__bernoulli( .2 ).returns( true );
    expected = { .type = e_native_unit_type::mounted_warrior,
                 .muskets_delta        = -1,
                 .horse_breeding_delta = -25 };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[tribe-arms] evolve_tribe_horse_breeding" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::sioux );

  auto f = [&] { evolve_tribe_horse_breeding( W.ss(), tribe ); };

  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  f();
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  tribe.horse_herds = 1;
  f();
  REQUIRE( tribe.horse_herds == 1 );
  REQUIRE( tribe.horse_breeding == 1 );

  f();
  REQUIRE( tribe.horse_herds == 1 );
  REQUIRE( tribe.horse_breeding == 2 );

  f();
  REQUIRE( tribe.horse_herds == 1 );
  REQUIRE( tribe.horse_breeding == 3 );

  tribe.horse_herds = 8;
  f();
  REQUIRE( tribe.horse_herds == 8 );
  REQUIRE( tribe.horse_breeding == 11 );

  tribe.horse_herds = 8;
  f();
  REQUIRE( tribe.horse_herds == 8 );
  REQUIRE( tribe.horse_breeding == 19 );

  tribe.horse_herds = 1000;
  f();
  REQUIRE( tribe.horse_herds == 1000 );
  REQUIRE( tribe.horse_breeding == 50 );

  REQUIRE( tribe.muskets == 0 );
}

TEST_CASE( "[tribe-arms] adjust_arms_on_dwelling_destruction" ) {
  World  W;
  Tribe& tribe = W.add_tribe( e_tribe::sioux );

  auto f = [&] {
    adjust_arms_on_dwelling_destruction( W.ss(), tribe );
  };

  REQUIRE( tribe.muskets == 0 );
  REQUIRE( tribe.horse_herds == 0 );
  REQUIRE( tribe.horse_breeding == 0 );

  SECTION( "dwellings=1" ) {
    W.create_n_dwellings( e_tribe::sioux, 1 );

    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 1;
    tribe.horse_herds    = 1;
    tribe.horse_breeding = 1;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 2;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 2;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 3;
    tribe.horse_herds    = 3;
    tribe.horse_breeding = 3;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 3;
    tribe.horse_herds    = 4;
    tribe.horse_breeding = 5;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
  }

  SECTION( "dwellings=2" ) {
    W.create_n_dwellings( e_tribe::sioux, 2 );

    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 1;
    tribe.horse_herds    = 1;
    tribe.horse_breeding = 1;
    f();
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 1 );

    tribe.muskets        = 2;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 2;
    f();
    REQUIRE( tribe.muskets == 2 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 1 );

    tribe.muskets        = 3;
    tribe.horse_herds    = 3;
    tribe.horse_breeding = 3;
    f();
    REQUIRE( tribe.muskets == 3 );
    REQUIRE( tribe.horse_herds == 2 );
    REQUIRE( tribe.horse_breeding == 2 );

    tribe.muskets        = 4;
    tribe.horse_herds    = 4;
    tribe.horse_breeding = 5;
    f();
    REQUIRE( tribe.muskets == 4 );
    REQUIRE( tribe.horse_herds == 2 );
    REQUIRE( tribe.horse_breeding == 3 );
  }

  SECTION( "dwellings=3" ) {
    W.create_n_dwellings( e_tribe::sioux, 3 );

    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 1;
    tribe.horse_herds    = 1;
    tribe.horse_breeding = 1;
    f();
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 1 );

    tribe.muskets        = 2;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 2;
    f();
    REQUIRE( tribe.muskets == 2 );
    REQUIRE( tribe.horse_herds == 2 );
    REQUIRE( tribe.horse_breeding == 2 );

    tribe.muskets        = 3;
    tribe.horse_herds    = 3;
    tribe.horse_breeding = 3;
    f();
    REQUIRE( tribe.muskets == 3 );
    REQUIRE( tribe.horse_herds == 2 );
    REQUIRE( tribe.horse_breeding == 2 );

    tribe.muskets        = 4;
    tribe.horse_herds    = 4;
    tribe.horse_breeding = 5;
    f();
    REQUIRE( tribe.muskets == 4 );
    REQUIRE( tribe.horse_herds == 3 );
    REQUIRE( tribe.horse_breeding == 4 );

    tribe.muskets        = 4;
    tribe.horse_herds    = 5;
    tribe.horse_breeding = 6;
    f();
    REQUIRE( tribe.muskets == 4 );
    REQUIRE( tribe.horse_herds == 4 );
    REQUIRE( tribe.horse_breeding == 4 );
  }

  SECTION( "dwellings=3" ) {
    W.create_n_dwellings( e_tribe::sioux, 4 );

    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    f();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );

    tribe.muskets        = 1;
    tribe.horse_herds    = 1;
    tribe.horse_breeding = 1;
    f();
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 1 );
    REQUIRE( tribe.horse_breeding == 1 );

    tribe.muskets        = 2;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 2;
    f();
    REQUIRE( tribe.muskets == 2 );
    REQUIRE( tribe.horse_herds == 2 );
    REQUIRE( tribe.horse_breeding == 2 );

    tribe.muskets        = 3;
    tribe.horse_herds    = 3;
    tribe.horse_breeding = 3;
    f();
    REQUIRE( tribe.muskets == 3 );
    REQUIRE( tribe.horse_herds == 3 );
    REQUIRE( tribe.horse_breeding == 3 );

    tribe.muskets        = 4;
    tribe.horse_herds    = 4;
    tribe.horse_breeding = 5;
    f();
    REQUIRE( tribe.muskets == 4 );
    REQUIRE( tribe.horse_herds == 3 );
    REQUIRE( tribe.horse_breeding == 4 );

    tribe.muskets        = 4;
    tribe.horse_herds    = 5;
    tribe.horse_breeding = 6;
    f();
    REQUIRE( tribe.muskets == 4 );
    REQUIRE( tribe.horse_herds == 4 );
    REQUIRE( tribe.horse_breeding == 5 );

    tribe.muskets        = 16;
    tribe.horse_herds    = 16;
    tribe.horse_breeding = 16;
    f();
    REQUIRE( tribe.muskets == 16 );
    REQUIRE( tribe.horse_herds == 12 );
    REQUIRE( tribe.horse_breeding == 12 );
  }
}

} // namespace
} // namespace rn
