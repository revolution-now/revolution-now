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

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_default_player(); }

  void create_map( vector<MapSquare> m ) {
    CHECK( width_ >= 0 );
    build_map( std::move( m ), width_ );
  }

  // Setting the width separately as opposed to passing it into
  // the create_map function allows better code formatting from
  // clang-format.
  void set_width( int width ) { width_ = width; }
  int  width_ = -1;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[connectivity] compute_terrain_connectivity" ) {
  World               W;
  TerrainConnectivity expected;
  MapSquare const     _ = W.make_ocean();
  MapSquare const     L = W.make_grassland();

  auto f = [&] {
    return compute_terrain_connectivity( W.ss() );
  };

  SECTION( "0x0" ) {
    W.set_width( 0 );
    W.create_map( {} );
    expected.x_size                         = 0;
    expected.indices                        = {};
    expected.indices_with_right_edge_access = {};
    expected.indices_with_left_edge_access  = {};
    REQUIRE( f() == expected );
  }

  SECTION( "1x1" ) {
    W.set_width( 1 );
    W.create_map( {
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
    W.set_width( 5 );
    W.create_map( {
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
    W.set_width( 1 );
    W.create_map( {
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
    W.set_width( 5 );
    W.create_map( {
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
    W.set_width( 5 );
    W.create_map( {
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
    W.set_width( 3 );
    W.create_map( {
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
    W.set_width( 6 );
    W.create_map( {
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
    W.set_width( 5 );
    W.create_map( {
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
    W.set_width( 5 );
    W.create_map( {
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
    W.set_width( 18 );
    W.create_map( {
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
  World           W;
  MapSquare const _ = W.make_ocean();
  MapSquare const L = W.make_grassland();
  W.set_width( 5 );
  W.create_map( {
      _, L, _, L, L, //
      L, L, L, L, L, //
      L, _, _, L, _, //
      L, _, L, L, _, //
      L, _, L, _, _, //
  } );
  W.update_terrain_connectivity();

  auto f = [&]( Coord coord ) {
    return colony_has_ocean_access( W.ss(), W.connectivity(),
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

} // namespace
} // namespace rn
