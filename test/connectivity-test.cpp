/****************************************************************
**connectivity.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-18.
*
* Description: Unit tests for the src/connectivity.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/connectivity.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/ref.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() { add_default_player(); }

  void create_map( vector<MapSquare> m ) {
    CHECK( width_ >= 0 );
    build_map( std::move( m ), width_ );
  }

  // Setting the width separately as opposed to passing it into
  // the create_map function allows better code formatting from
  // clang-format.
  void set_width( int width ) { width_ = width; }
  int width_ = -1;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[connectivity] compute_terrain_connectivity" ) {
  world w;
  TerrainConnectivity expected;
  MapSquare const _ = w.make_ocean();
  MapSquare const L = w.make_grassland();

  auto f = [&] {
    return compute_terrain_connectivity( w.ss() );
  };

  SECTION( "0x0" ) {
    w.set_width( 0 );
    w.create_map( {} );
    expected.x_size                         = 0;
    expected.indices                        = {};
    expected.indices_with_right_edge_access = {};
    expected.indices_with_left_edge_access  = {};
    REQUIRE( f() == expected );
  }

  SECTION( "1x1" ) {
    w.set_width( 1 );
    w.create_map( {
      _, //
    } );
    expected.x_size  = 1;
    expected.indices = {
      1, //
    };
    expected.indices_with_right_edge_access = { 1 };
    expected.indices_with_left_edge_access  = { 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "5x1 all connected" ) {
    w.set_width( 5 );
    w.create_map( {
      L, L, L, L, L, //
    } );
    expected.x_size  = 5;
    expected.indices = {
      1, 1, 1, 1, 1, //
    };
    expected.indices_with_right_edge_access = { 1 };
    expected.indices_with_left_edge_access  = { 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "1x5 all connected" ) {
    w.set_width( 1 );
    w.create_map( {
      _, //
      _, //
      _, //
      _, //
      _, //
    } );
    expected.x_size  = 1;
    expected.indices = {
      1, //
      1, //
      1, //
      1, //
      1, //
    };
    expected.indices_with_right_edge_access = { 1 };
    expected.indices_with_left_edge_access  = { 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "5x1 alternating" ) {
    w.set_width( 5 );
    w.create_map( {
      L, _, L, _, L, //
    } );
    expected.x_size  = 5;
    expected.indices = {
      1, 2, 3, 4, 5, //
    };
    expected.indices_with_right_edge_access = { 5 };
    expected.indices_with_left_edge_access  = { 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "5x2 checkers" ) {
    w.set_width( 5 );
    w.create_map( {
      L, _, L, _, L, //
      _, L, _, L, _, //
    } );
    expected.x_size  = 5;
    expected.indices = {
      1, 2, 1, 2, 1, //
      2, 1, 2, 1, 2, //
    };
    expected.indices_with_right_edge_access = { 1, 2 };
    expected.indices_with_left_edge_access  = { 1, 2 };
    REQUIRE( f() == expected );
  }

  SECTION( "3x3" ) {
    w.set_width( 3 );
    w.create_map( {
      _, L, _, //
      L, L, L, //
      _, L, L, //
    } );
    expected.x_size  = 3;
    expected.indices = {
      1, 2, 3, //
      2, 2, 2, //
      4, 2, 2, //
    };
    expected.indices_with_right_edge_access = { 2, 3 };
    expected.indices_with_left_edge_access  = { 1, 2, 4 };
    REQUIRE( f() == expected );
  }

  SECTION( "6x3" ) {
    w.set_width( 6 );
    w.create_map( {
      _, L, _, L, L, L, //
      L, L, L, L, L, L, //
      _, L, L, L, L, L, //
    } );
    expected.x_size  = 6;
    expected.indices = {
      1, 2, 3, 2, 2, 2, //
      2, 2, 2, 2, 2, 2, //
      4, 2, 2, 2, 2, 2, //
    };
    expected.indices_with_right_edge_access = { 2 };
    expected.indices_with_left_edge_access  = { 1, 2, 4 };
    REQUIRE( f() == expected );
  }

  SECTION( "5x5 all connected" ) {
    w.set_width( 5 );
    w.create_map( {
      _, _, _, _, _, //
      _, _, _, _, _, //
      _, _, _, _, _, //
      _, _, _, _, _, //
      _, _, _, _, _, //
    } );
    expected.x_size  = 5;
    expected.indices = {
      1, 1, 1, 1, 1, //
      1, 1, 1, 1, 1, //
      1, 1, 1, 1, 1, //
      1, 1, 1, 1, 1, //
      1, 1, 1, 1, 1, //
    };
    expected.indices_with_right_edge_access = { 1 };
    expected.indices_with_left_edge_access  = { 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "5x5 split" ) {
    w.set_width( 5 );
    w.create_map( {
      _, _, _, _, _, //
      _, _, _, L, _, //
      _, _, L, L, L, //
      _, L, L, _, _, //
      L, L, _, _, L, //
    } );
    expected.x_size  = 5;
    expected.indices = {
      1, 1, 1, 1, 1, //
      1, 1, 1, 2, 1, //
      1, 1, 2, 2, 2, //
      1, 2, 2, 3, 3, //
      2, 2, 3, 3, 4, //
    };
    expected.indices_with_right_edge_access = { 1, 2, 3, 4 };
    expected.indices_with_left_edge_access  = { 1, 2 };
    REQUIRE( f() == expected );
  }

  SECTION( "large" ) {
    w.set_width( 18 );
    w.create_map( {
      L, _, _, _, _, L, _, _, _, _, _, L, _, _, _, _, L, _, //
      _, _, _, _, _, L, _, _, _, _, L, L, L, _, _, _, L, L, //
      _, L, L, _, _, L, _, _, _, _, _, L, _, _, _, _, _, _, //
      _, L, L, _, L, L, L, L, _, _, _, L, _, _, _, _, _, _, //
      _, _, _, _, L, _, _, L, _, _, L, _, _, _, _, _, L, L, //
      _, _, _, _, L, _, L, L, _, L, _, _, _, _, _, L, L, _, //
      _, _, _, _, L, L, L, L, _, L, _, _, _, _, _, L, _, _, //
      _, _, _, _, L, L, _, _, _, L, L, L, L, _, _, L, _, _, //
      _, _, _, _, _, L, _, _, _, L, L, L, L, L, _, L, L, _, //
      L, L, L, _, _, L, _, _, _, _, _, L, L, _, _, _, L, L, //
      L, _, _, _, _, L, L, _, _, _, _, _, _, _, _, _, _, _, //
      _, _, _, _, _, L, L, L, _, _, _, _, L, L, _, _, _, _, //
      _, _, _, _, _, L, _, L, L, _, _, L, _, _, L, _, _, _, //
      _, _, _, _, L, L, _, _, L, _, _, _, L, L, _, _, _, _, //
      _, _, _, _, L, _, _, _, L, _, _, _, _, L, _, _, _, _, //
      _, _, _, _, L, L, L, L, L, L, _, _, _, _, _, _, _, _, //
      _, L, L, _, _, _, _, _, _, L, L, L, L, _, L, L, L, _, //
      _, L, L, L, _, _, _, _, _, _, _, L, L, L, L, _, L, L, //
      _, L, _, L, _, _, _, _, _, _, _, _, _, _, L, _, _, _, //
      _, L, L, L, _, _, _, _, _, _, _, _, _, _, L, _, _, _, //
    } );
    expected.x_size = 18;
    // clang-format off
    expected.indices = {
        1, 2, 2, 2, 2, 3, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4, 6, 7,
        2, 2, 2, 2, 2, 3, 4, 4, 4, 4, 5, 5, 5, 4, 4, 4, 6, 6,
        2, 8, 8, 2, 2, 3, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4, 4, 4,
        2, 8, 8, 2, 3, 3, 3, 3, 4, 4, 4, 5, 4, 4, 4, 4, 4, 4,
        2, 2, 2, 2, 3, 9, 9, 3, 4, 4, 5, 4, 4, 4, 4, 4,10,10,
        2, 2, 2, 2, 3, 9, 3, 3, 4, 5, 4, 4, 4, 4, 4,10,10,11,
        2, 2, 2, 2, 3, 3, 3, 3, 4, 5, 4, 4, 4, 4, 4,10,11,11,
        2, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 4, 4,10,11,11,
        2, 2, 2, 2, 2, 3, 4, 4, 4, 5, 5, 5, 5, 5, 4,10,10,11,
       12,12,12, 2, 2, 3, 4, 4, 4, 4, 4, 5, 5, 4, 4, 4,10,10,
       12, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4,13,13, 4, 4, 4, 4,
        2, 2, 2, 2, 2, 3,14, 3, 3, 4, 4,13, 4, 4,13, 4, 4, 4,
        2, 2, 2, 2, 3, 3,14,14, 3, 4, 4, 4,13,13, 4, 4, 4, 4,
        2, 2, 2, 2, 3,14,14,14, 3, 4, 4, 4, 4,13, 4, 4, 4, 4,
        2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
        2,15,15, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 3, 3, 3, 4,
        2,15,15,15, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,16, 3, 3,
        2,15,17,15, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,16,16,16,
        2,15,15,15, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,16,16,16,
    };
    // clang-format on
    expected.indices_with_right_edge_access = { 7,  6, 4, 10,
                                                11, 3, 16 };
    expected.indices_with_left_edge_access  = { 1, 2, 12 };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[connectivity] water_square_has_ocean_access" ) {
  TerrainConnectivity const connectivity{
    .x_size = 5,
    // NOTE: this map of indices is not internally consistent,
    // but that's ok for this test.
    .indices =
        {
          1, 2, 5, 2, 2, //
          2, 2, 2, 2, 2, //
          2, 6, 6, 2, 4, //
          2, 6, 6, 2, 3, //
          2, 2, 2, 2, 3, //
        },
    .indices_with_right_edge_access = { 2, 3, 4 },
    .indices_with_left_edge_access  = { 1, 2 } };

  SECTION( "left or right" ) {
    auto f = [&]( Coord coord ) {
      return water_square_has_ocean_access( connectivity,
                                            coord );
    };

    auto g = [&]( Coord coord ) {
      return is_inland_lake( connectivity, coord );
    };

    REQUIRE( f( { .x = 0, .y = 0 } ) );
    REQUIRE( f( { .x = 1, .y = 0 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 0 } ) );
    REQUIRE( f( { .x = 3, .y = 0 } ) );
    REQUIRE( f( { .x = 4, .y = 0 } ) );
    REQUIRE( f( { .x = 0, .y = 1 } ) );
    REQUIRE( f( { .x = 1, .y = 1 } ) );
    REQUIRE( f( { .x = 2, .y = 1 } ) );
    REQUIRE( f( { .x = 3, .y = 1 } ) );
    REQUIRE( f( { .x = 4, .y = 1 } ) );
    REQUIRE( f( { .x = 0, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 1, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 2 } ) );
    REQUIRE( f( { .x = 3, .y = 2 } ) );
    REQUIRE( f( { .x = 4, .y = 2 } ) );
    REQUIRE( f( { .x = 0, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 1, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 3 } ) );
    REQUIRE( f( { .x = 3, .y = 3 } ) );
    REQUIRE( f( { .x = 4, .y = 3 } ) );
    REQUIRE( f( { .x = 0, .y = 4 } ) );
    REQUIRE( f( { .x = 1, .y = 4 } ) );
    REQUIRE( f( { .x = 2, .y = 4 } ) );
    REQUIRE( f( { .x = 3, .y = 4 } ) );
    REQUIRE( f( { .x = 4, .y = 4 } ) );

    REQUIRE_FALSE( g( { .x = 0, .y = 0 } ) );
    REQUIRE_FALSE( g( { .x = 1, .y = 0 } ) );
    REQUIRE( g( { .x = 2, .y = 0 } ) );
    REQUIRE_FALSE( g( { .x = 3, .y = 0 } ) );
    REQUIRE_FALSE( g( { .x = 4, .y = 0 } ) );
    REQUIRE_FALSE( g( { .x = 0, .y = 1 } ) );
    REQUIRE_FALSE( g( { .x = 1, .y = 1 } ) );
    REQUIRE_FALSE( g( { .x = 2, .y = 1 } ) );
    REQUIRE_FALSE( g( { .x = 3, .y = 1 } ) );
    REQUIRE_FALSE( g( { .x = 4, .y = 1 } ) );
    REQUIRE_FALSE( g( { .x = 0, .y = 2 } ) );
    REQUIRE( g( { .x = 1, .y = 2 } ) );
    REQUIRE( g( { .x = 2, .y = 2 } ) );
    REQUIRE_FALSE( g( { .x = 3, .y = 2 } ) );
    REQUIRE_FALSE( g( { .x = 4, .y = 2 } ) );
    REQUIRE_FALSE( g( { .x = 0, .y = 3 } ) );
    REQUIRE( g( { .x = 1, .y = 3 } ) );
    REQUIRE( g( { .x = 2, .y = 3 } ) );
    REQUIRE_FALSE( g( { .x = 3, .y = 3 } ) );
    REQUIRE_FALSE( g( { .x = 4, .y = 3 } ) );
    REQUIRE_FALSE( g( { .x = 0, .y = 4 } ) );
    REQUIRE_FALSE( g( { .x = 1, .y = 4 } ) );
    REQUIRE_FALSE( g( { .x = 2, .y = 4 } ) );
    REQUIRE_FALSE( g( { .x = 3, .y = 4 } ) );
    REQUIRE_FALSE( g( { .x = 4, .y = 4 } ) );
  }

  SECTION( "left" ) {
    auto f = [&]( Coord coord ) {
      return water_square_has_left_ocean_access( connectivity,
                                                 coord );
    };

    REQUIRE( f( { .x = 0, .y = 0 } ) );
    REQUIRE( f( { .x = 1, .y = 0 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 0 } ) );
    REQUIRE( f( { .x = 3, .y = 0 } ) );
    REQUIRE( f( { .x = 4, .y = 0 } ) );
    REQUIRE( f( { .x = 0, .y = 1 } ) );
    REQUIRE( f( { .x = 1, .y = 1 } ) );
    REQUIRE( f( { .x = 2, .y = 1 } ) );
    REQUIRE( f( { .x = 3, .y = 1 } ) );
    REQUIRE( f( { .x = 4, .y = 1 } ) );
    REQUIRE( f( { .x = 0, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 1, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 2 } ) );
    REQUIRE( f( { .x = 3, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 4, .y = 2 } ) );
    REQUIRE( f( { .x = 0, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 1, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 3 } ) );
    REQUIRE( f( { .x = 3, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 4, .y = 3 } ) );
    REQUIRE( f( { .x = 0, .y = 4 } ) );
    REQUIRE( f( { .x = 1, .y = 4 } ) );
    REQUIRE( f( { .x = 2, .y = 4 } ) );
    REQUIRE( f( { .x = 3, .y = 4 } ) );
    REQUIRE_FALSE( f( { .x = 4, .y = 4 } ) );
  }

  SECTION( "right" ) {
    auto f = [&]( Coord coord ) {
      return water_square_has_right_ocean_access( connectivity,
                                                  coord );
    };

    REQUIRE_FALSE( f( { .x = 0, .y = 0 } ) );
    REQUIRE( f( { .x = 1, .y = 0 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 0 } ) );
    REQUIRE( f( { .x = 3, .y = 0 } ) );
    REQUIRE( f( { .x = 4, .y = 0 } ) );
    REQUIRE( f( { .x = 0, .y = 1 } ) );
    REQUIRE( f( { .x = 1, .y = 1 } ) );
    REQUIRE( f( { .x = 2, .y = 1 } ) );
    REQUIRE( f( { .x = 3, .y = 1 } ) );
    REQUIRE( f( { .x = 4, .y = 1 } ) );
    REQUIRE( f( { .x = 0, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 1, .y = 2 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 2 } ) );
    REQUIRE( f( { .x = 3, .y = 2 } ) );
    REQUIRE( f( { .x = 4, .y = 2 } ) );
    REQUIRE( f( { .x = 0, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 1, .y = 3 } ) );
    REQUIRE_FALSE( f( { .x = 2, .y = 3 } ) );
    REQUIRE( f( { .x = 3, .y = 3 } ) );
    REQUIRE( f( { .x = 4, .y = 3 } ) );
    REQUIRE( f( { .x = 0, .y = 4 } ) );
    REQUIRE( f( { .x = 1, .y = 4 } ) );
    REQUIRE( f( { .x = 2, .y = 4 } ) );
    REQUIRE( f( { .x = 3, .y = 4 } ) );
    REQUIRE( f( { .x = 4, .y = 4 } ) );
  }
}

TEST_CASE( "[connectivity] colony_has_ocean_access" ) {
  world w;
  MapSquare const _ = w.make_ocean();
  MapSquare const L = w.make_grassland();
  w.set_width( 5 );
  w.create_map( {
    _, L, _, L, L, //
    L, L, L, L, L, //
    L, _, _, L, _, //
    L, _, L, L, _, //
    L, _, L, _, _, //
  } );
  w.update_terrain_connectivity();

  auto f = [&]( Coord coord ) {
    return colony_has_ocean_access( w.ss(), w.connectivity(),
                                    coord );
  };

  // water: { .x = 0, .y = 0 }
  REQUIRE( f( { .x = 1, .y = 0 } ) );
  // water: { .x = 2, .y = 0 }
  REQUIRE_FALSE( f( { .x = 3, .y = 0 } ) );
  REQUIRE_FALSE( f( { .x = 4, .y = 0 } ) );
  REQUIRE( f( { .x = 0, .y = 1 } ) );
  REQUIRE( f( { .x = 1, .y = 1 } ) );
  REQUIRE_FALSE( f( { .x = 2, .y = 1 } ) );
  REQUIRE( f( { .x = 3, .y = 1 } ) );
  REQUIRE( f( { .x = 4, .y = 1 } ) );
  REQUIRE_FALSE( f( { .x = 0, .y = 2 } ) );
  // water: { .x = 1, .y = 2 }
  // water: { .x = 2, .y = 2 }
  REQUIRE( f( { .x = 3, .y = 2 } ) );
  // water: { .x = 4, .y = 2 }
  REQUIRE_FALSE( f( { .x = 0, .y = 3 } ) );
  // water: { .x = 1, .y = 3 }
  REQUIRE( f( { .x = 2, .y = 3 } ) );
  REQUIRE( f( { .x = 3, .y = 3 } ) );
  // water: { .x = 4, .y = 3 }
  REQUIRE_FALSE( f( { .x = 0, .y = 4 } ) );
  // water: { .x = 1, .y = 4 }
  REQUIRE( f( { .x = 2, .y = 4 } ) );
  // water: { .x = 3, .y = 4 }
  // water: { .x = 4, .y = 4 }
}

TEST_CASE( "[connectivity] tiles_are_connected" ) {
  world w;
  MapSquare const _ = w.make_ocean();
  MapSquare const L = w.make_grassland();

  w.set_width( 18 );
  // clang-format off
  w.create_map( { /*
    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  g  h  */
    L, _, _, _, _, L, _, _, _, _, _, L, _, _, _, _, L, _, // 0
    _, _, _, _, _, L, _, _, _, _, L, L, L, _, _, _, L, L, // 1
    _, L, L, _, _, L, _, _, _, _, _, L, _, _, _, _, _, _, // 2
    _, L, L, _, L, L, L, L, _, _, _, L, _, _, _, _, _, _, // 3
    _, _, _, _, L, _, _, L, _, _, L, _, _, _, _, _, L, L, // 4
    _, _, _, _, L, _, L, L, _, L, _, _, _, _, _, L, L, _, // 5
    _, _, _, _, L, L, L, L, _, L, _, _, _, _, _, L, _, _, // 6
    _, _, _, _, L, L, _, _, _, L, L, L, L, _, _, L, _, _, // 7
    _, _, _, _, _, L, _, _, _, L, L, L, L, L, _, L, L, _, // 8
    L, L, L, _, _, L, _, _, _, _, _, L, L, _, _, _, L, L, // 9
    L, _, _, _, _, L, L, _, _, _, _, _, _, _, _, _, _, _, // a
    _, _, _, _, _, L, L, L, _, _, _, _, L, L, _, _, _, _, // b
    _, _, _, _, _, L, _, L, L, _, _, L, _, _, L, _, _, _, // c
    _, _, _, _, L, L, _, _, L, _, _, _, L, L, _, _, _, _, // d
    _, _, _, _, L, _, _, _, L, _, _, _, _, L, _, _, _, _, // e
    _, _, _, _, L, L, L, L, L, L, _, _, _, _, _, _, _, _, // f
    _, L, L, _, _, _, _, _, _, L, L, L, L, _, L, L, L, _, // g
    _, L, L, L, _, _, _, _, _, _, _, L, L, L, L, _, L, L, // h
    _, L, _, L, _, _, _, _, _, _, _, _, _, _, L, _, _, _, // i
    _, L, L, L, _, _, _, _, _, _, _, _, _, _, L, _, _, _, /* j
    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  g  h  */
  } );
  // clang-format on

  TerrainConnectivity const connectivity =
      compute_terrain_connectivity( w.ss() );

  auto const f = [&] [[clang::noinline]] ( point const p1,
                                           point const p2 ) {
    return tiles_are_connected( connectivity, p1, p2 );
  };

  REQUIRE( f( { .x = 0, .y = 0 }, { .x = 0, .y = 0 } ) );
  REQUIRE( !f( { .x = 1, .y = 0 }, { .x = 0, .y = 0 } ) );
  REQUIRE( !f( { .x = 0, .y = 0 }, { .x = 1, .y = 2 } ) );
  REQUIRE( f( { .x = 2, .y = 3 }, { .x = 1, .y = 2 } ) );
  REQUIRE( f( { .x = 2, .y = 3 }, { .x = 1, .y = 2 } ) );
  REQUIRE( f( { .x = 5, .y = 1 }, { .x = 14, .y = 16 } ) );
  REQUIRE( !f( { .x = 5, .y = 1 }, { .x = 14, .y = 13 } ) );
  REQUIRE( !f( { .x = 5, .y = 1 }, { .x = 14, .y = 12 } ) );
  REQUIRE( !f( { .x = 11, .y = 0 }, { .x = 14, .y = 12 } ) );
  REQUIRE( f( { .x = 11, .y = 12 }, { .x = 14, .y = 12 } ) );
  REQUIRE( f( { .x = 15, .y = 5 }, { .x = 17, .y = 9 } ) );
  REQUIRE( f( { .x = 2, .y = 9 }, { .x = 0, .y = 10 } ) );
}

} // namespace
} // namespace rn
