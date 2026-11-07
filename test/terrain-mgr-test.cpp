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
  vector<point> visited;
  visited.reserve( w.terrain().world_size_tiles().area() );

  // clang-format off
  vector<point> const expected{
    { .x = 0, .y = 0 },  { .x = 1, .y = 0 }, { .x = 2, .y = 0 },  { .x = 3, .y = 0 },
    { .x = 4, .y = 0 },  { .x = 5, .y = 0 }, { .x = 6, .y = 0 },  { .x = 7, .y = 0 },
    { .x = 0, .y = 1 },  { .x = 1, .y = 1 }, { .x = 2, .y = 1 },  { .x = 3, .y = 1 },
    { .x = 4, .y = 1 },  { .x = 5, .y = 1 }, { .x = 6, .y = 1 },  { .x = 7, .y = 1 },
    { .x = 0, .y = 2 },  { .x = 1, .y = 2 }, { .x = 2, .y = 2 },  { .x = 3, .y = 2 },
    { .x = 4, .y = 2 },  { .x = 5, .y = 2 }, { .x = 6, .y = 2 },  { .x = 7, .y = 2 },
    { .x = 0, .y = 3 },  { .x = 1, .y = 3 }, { .x = 2, .y = 3 },  { .x = 3, .y = 3 },
    { .x = 4, .y = 3 },  { .x = 5, .y = 3 }, { .x = 6, .y = 3 },  { .x = 7, .y = 3 },
    { .x = 0, .y = 4 },  { .x = 1, .y = 4 }, { .x = 2, .y = 4 },  { .x = 3, .y = 4 },
    { .x = 4, .y = 4 },  { .x = 5, .y = 4 }, { .x = 6, .y = 4 },  { .x = 7, .y = 4 },
    { .x = 0, .y = 5 },  { .x = 1, .y = 5 }, { .x = 2, .y = 5 },  { .x = 3, .y = 5 },
    { .x = 4, .y = 5 },  { .x = 5, .y = 5 }, { .x = 6, .y = 5 },  { .x = 7, .y = 5 },
    { .x = 0, .y = 6 },  { .x = 1, .y = 6 }, { .x = 2, .y = 6 },  { .x = 3, .y = 6 },
    { .x = 4, .y = 6 },  { .x = 5, .y = 6 }, { .x = 6, .y = 6 },  { .x = 7, .y = 6 },
    { .x = 0, .y = 7 },  { .x = 1, .y = 7 }, { .x = 2, .y = 7 },  { .x = 3, .y = 7 },
    { .x = 4, .y = 7 },  { .x = 5, .y = 7 }, { .x = 6, .y = 7 },  { .x = 7, .y = 7 },
    { .x = 0, .y = 8 },  { .x = 1, .y = 8 }, { .x = 2, .y = 8 },  { .x = 3, .y = 8 },
    { .x = 4, .y = 8 },  { .x = 5, .y = 8 }, { .x = 6, .y = 8 },  { .x = 7, .y = 8 },
    { .x = 0, .y = 9 },  { .x = 1, .y = 9 }, { .x = 2, .y = 9 },  { .x = 3, .y = 9 },
    { .x = 4, .y = 9 },  { .x = 5, .y = 9 }, { .x = 6, .y = 9 },  { .x = 7, .y = 9 },
    { .x = 0, .y = 10 }, { .x = 1, .y = 10 }, { .x = 2, .y = 10 }, { .x = 3, .y = 10 },
    { .x = 4, .y = 10 }, { .x = 5, .y = 10 }, { .x = 6, .y = 10 }, { .x = 7, .y = 10 },
  };
  // clang-format on

  // Variant 1.
  visited.clear();
  on_all_tiles(
      w.ss(), [&]( point const p, MapSquare const& square ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
      } );
  REQUIRE( visited == expected );

  // Variant 2.
  visited.clear();
  on_all_tiles(
      w.terrain().world_map(),
      [&]( point const p, MapSquare const& square ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
      } );
  REQUIRE( visited == expected );

  // Variant 3.
  visited.clear();
  MapMatrix m = w.terrain().world_map();
  on_all_tiles( m, [&]( point const p, MapSquare& square ) {
    BASE_CHECK( w.terrain().square_at( p ) == square );
    visited.push_back( p );
  } );
  REQUIRE( visited == expected );
}

TEST_CASE( "[terrain-mgr] on_surrounding" ) {
  world w;
  vector<point> visited, expected;
  visited.reserve( 8 );
  point center;

  center = { .x = 2, .y = 3 };
  // clang-format off
  expected = {
    { .x = 1, .y = 2 }, { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
    { .x = 1, .y = 3 },                     { .x = 3, .y = 3 },
    { .x = 1, .y = 4 }, { .x = 2, .y = 4 }, { .x = 3, .y = 4 },
  };
  // clang-format on

  // Variant 1.
  visited.clear();
  on_surrounding(
      w.ss(), center,
      [&]( point const p, MapSquare const& square ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
      } );
  REQUIRE( visited == expected );

  // Variant 2.
  visited.clear();
  on_surrounding(
      w.terrain().world_map(), center,
      [&]( point const p, MapSquare const& square ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
      } );
  REQUIRE( visited == expected );

  center = { .x = 7, .y = 0 };
  // clang-format off
  expected = {
    { .x = 6, .y = 0 },
    { .x = 6, .y = 1 }, { .x = 7, .y = 1 },
  };
  // clang-format on

  // Variant 1.
  visited.clear();
  on_surrounding(
      w.ss(), center,
      [&]( point const p, MapSquare const& square ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
      } );
  REQUIRE( visited == expected );

  // Variant 2.
  visited.clear();
  on_surrounding(
      w.terrain().world_map(), center,
      [&]( point const p, MapSquare const& square ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
      } );
  REQUIRE( visited == expected );
}

TEST_CASE( "[terrain-mgr] on_surrounding_cardinal" ) {
  world w;
  vector<point> visited, expected;
  visited.reserve( 8 );
  vector<e_cardinal_direction> directions, expected_directions;
  directions.reserve( 4 );
  point center;

  center = { .x = 2, .y = 3 };
  // clang-format off
  expected = {
                        { .x = 2, .y = 2 },
    { .x = 1, .y = 3 },                     { .x = 3, .y = 3 },
                        { .x = 2, .y = 4 },
  };
  // clang-format on
  expected_directions = {
    e_cardinal_direction::n,
    e_cardinal_direction::w,
    e_cardinal_direction::e,
    e_cardinal_direction::s,
  };

  // Variant 1.
  visited.clear();
  directions.clear();
  on_surrounding_cardinal(
      w.terrain().world_map(), center,
      [&]( point const p, MapSquare const& square,
           e_cardinal_direction const d ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
        directions.push_back( d );
      } );
  REQUIRE( visited == expected );
  REQUIRE( directions == expected_directions );

  center = { .x = 7, .y = 0 };
  // clang-format off
  expected = {
    { .x = 6, .y = 0 },
                        { .x = 7, .y = 1 },
  };
  // clang-format on
  expected_directions = {
    e_cardinal_direction::w,
    e_cardinal_direction::s,
  };

  // Variant 1.
  visited.clear();
  directions.clear();
  on_surrounding_cardinal(
      w.terrain().world_map(), center,
      [&]( point const p, MapSquare const& square,
           e_cardinal_direction const d ) {
        BASE_CHECK( w.terrain().square_at( p ) == square );
        visited.push_back( p );
        directions.push_back( d );
      } );
  REQUIRE( visited == expected );
  REQUIRE( directions == expected_directions );
}

TEST_CASE( "[terrain-mgr] assign_formation" ) {
  world w;
  MapSquare square;
  square.surface = e_surface::land;

  using enum e_terrain_formation;

  auto const f = [&] [[clang::noinline]] (
                     e_terrain_formation const formation ) {
    assign_formation( square, formation );
  };

  REQUIRE( square.overlay == nothing );
  f( hills );
  REQUIRE( square.overlay == e_land_overlay::hills );
  f( clearing );
  REQUIRE( square.overlay == nothing );
  f( mountains );
  REQUIRE( square.overlay == e_land_overlay::mountains );
  f( clearing );
  REQUIRE( square.overlay == nothing );
}

} // namespace
} // namespace rn
