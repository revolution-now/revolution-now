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

// ss
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/tribe.rds.hpp"

// gfx
#include "src/gfx/iter.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

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

  auto create_n_dwellings = [&]( int n ) {
    BASE_CHECK( tribe != nullptr );
    gfx::rect_iterator const ri(
        W.terrain().world_rect_tiles().to_gfx() );
    for( Coord const coord : ri ) {
      if( n-- <= 0 ) return;
      W.add_dwelling( coord, tribe->type );
    }
  };

  // semi_nomadic: { N=0, M=6,  A=0 }
  SECTION( "semi-nomadic" ) {
    tribe = &W.add_tribe( e_tribe::sioux );

    SECTION( "dwellings=0" ) { // max = 50
      create_n_dwellings( 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 50 );
    }

    SECTION( "dwellings=1" ) { // max = 56
      create_n_dwellings( 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 56 );
    }

    SECTION( "dwellings=10" ) { // max = 110
      create_n_dwellings( 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 110 );
    }
  }

  // agrarian: { N=4, M=10, A=0 }
  SECTION( "agrarian" ) {
    tribe = &W.add_tribe( e_tribe::cherokee );

    SECTION( "dwellings=0" ) { // max = 54
      create_n_dwellings( 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 54 );
    }

    SECTION( "dwellings=1" ) { // max = 64
      create_n_dwellings( 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 64 );
    }

    SECTION( "dwellings=10" ) { // max = 154
      create_n_dwellings( 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 154 );
    }
  }

  // advanced: { N=6, M=14, A=0 }
  SECTION( "aztec" ) {
    tribe = &W.add_tribe( e_tribe::aztec );

    SECTION( "dwellings=0" ) { // max = 56
      create_n_dwellings( 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 56 );
    }

    SECTION( "dwellings=1" ) { // max = 70
      create_n_dwellings( 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 70 );
    }

    SECTION( "dwellings=10" ) { // max = 196
      create_n_dwellings( 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 196 );
    }
  }

  // civilized: { N=8, M=18, A=0 }
  SECTION( "inca" ) {
    tribe = &W.add_tribe( e_tribe::inca );

    SECTION( "dwellings=0" ) { // max = 58
      create_n_dwellings( 0 );
      tribe->horse_breeding = 45;
      f();
      REQUIRE( tribe->horse_breeding == 58 );
    }

    SECTION( "dwellings=1" ) { // max = 76
      create_n_dwellings( 1 );
      tribe->horse_breeding = 51;
      f();
      REQUIRE( tribe->horse_breeding == 76 );
    }

    SECTION( "dwellings=10" ) { // max = 238
      create_n_dwellings( 10 );
      tribe->horse_breeding = 450;
      f();
      REQUIRE( tribe->horse_breeding == 238 );
    }
  }

  REQUIRE( tribe->horse_herds == 0 );
}

} // namespace
} // namespace rn
