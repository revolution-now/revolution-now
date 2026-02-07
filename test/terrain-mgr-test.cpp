/****************************************************************
**terrain-mgr-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-14.
*
* Description: Unit tests for the terrain-mgr module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/terrain-mgr.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    using enum e_player;
    add_player( english );
    set_default_player_type( english );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const S = make_ocean();
    static MapSquare const _ = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{/*
          0 1 2 3 4 5 6 7
      0*/ S,_,_,_,_,_,_,S, /*0
      1*/ S,_,_,_,_,_,_,S, /*1
      2*/ S,_,S,S,_,_,_,S, /*2
      3*/ S,_,_,S,_,_,_,S, /*3
      4*/ S,_,_,_,_,_,_,S, /*4
      5*/ S,_,S,S,S,_,_,S, /*5
      6*/ S,_,S,_,S,S,S,S, /*6
      7*/ S,_,S,_,S,_,S,S, /*7
      8*/ S,S,S,S,S,S,S,S, /*8
      9*/ S,S,S,_,S,S,S,S, /*9
      a*/ S,S,S,S,S,S,S,S, /*a
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[terrain-mgr] num_surrounding_land_tiles/SS" ) {
  world w;
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return num_surrounding_land_tiles( w.ss(), tile );
  };

  tile = { .x = 1, .y = 0 };
  REQUIRE( f() == 3 );

  tile = { .x = 2, .y = 3 };
  REQUIRE( f() == 5 );

  tile = { .x = 3, .y = 6 };
  REQUIRE( f() == 1 );

  tile = { .x = 3, .y = 7 };
  REQUIRE( f() == 1 );

  tile = { .x = 5, .y = 7 };
  REQUIRE( f() == 0 );

  tile = { .x = 5, .y = 2 };
  REQUIRE( f() == 8 );
}

TEST_CASE( "[terrain-mgr] is_island/SS" ) {
  world w;
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return is_island( w.ss(), tile );
  };

  tile = { .x = 1, .y = 0 };
  REQUIRE( f() == false );

  tile = { .x = 2, .y = 3 };
  REQUIRE( f() == false );

  tile = { .x = 3, .y = 6 };
  REQUIRE( f() == false );

  tile = { .x = 3, .y = 7 };
  REQUIRE( f() == false );

  tile = { .x = 5, .y = 7 };
  REQUIRE( f() == true );

  tile = { .x = 5, .y = 2 };
  REQUIRE( f() == false );

  // Make sure that we check the tile itself and not just the
  // surroundings before concluding that it is an island.
  BASE_CHECK( w.square( { .x = 1, .y = 9 } ).surface ==
              e_surface::water );
  tile = { .x = 1, .y = 9 };
  REQUIRE( f() == false );

  BASE_CHECK( w.square( { .x = 3, .y = 9 } ).surface ==
              e_surface::land );
  tile = { .x = 3, .y = 9 };
  REQUIRE( f() == true );
}

TEST_CASE(
    "[terrain-mgr] num_surrounding_land_tiles/RealTerrain" ) {
  world w;
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return num_surrounding_land_tiles(
        w.terrain().real_terrain(), tile );
  };

  tile = { .x = 1, .y = 0 };
  REQUIRE( f() == 3 );

  tile = { .x = 2, .y = 3 };
  REQUIRE( f() == 5 );

  tile = { .x = 3, .y = 6 };
  REQUIRE( f() == 1 );

  tile = { .x = 3, .y = 7 };
  REQUIRE( f() == 1 );

  tile = { .x = 5, .y = 7 };
  REQUIRE( f() == 0 );

  tile = { .x = 5, .y = 2 };
  REQUIRE( f() == 8 );
}

TEST_CASE( "[terrain-mgr] is_island/RealTerrain" ) {
  world w;
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return is_island( w.terrain().real_terrain(), tile );
  };

  tile = { .x = 1, .y = 0 };
  REQUIRE( f() == false );

  tile = { .x = 2, .y = 3 };
  REQUIRE( f() == false );

  tile = { .x = 3, .y = 6 };
  REQUIRE( f() == false );

  tile = { .x = 3, .y = 7 };
  REQUIRE( f() == false );

  tile = { .x = 5, .y = 7 };
  REQUIRE( f() == true );

  tile = { .x = 5, .y = 2 };
  REQUIRE( f() == false );

  // Make sure that we check the tile itself and not just the
  // surroundings before concluding that it is an island.
  BASE_CHECK( w.square( { .x = 1, .y = 9 } ).surface ==
              e_surface::water );
  tile = { .x = 1, .y = 9 };
  REQUIRE( f() == false );

  BASE_CHECK( w.square( { .x = 3, .y = 9 } ).surface ==
              e_surface::land );
  tile = { .x = 3, .y = 9 };
  REQUIRE( f() == true );
}

TEST_CASE( "[terrain-mgr] on_all_tiles" ) {
  world w;
}

} // namespace
} // namespace rn
