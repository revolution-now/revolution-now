/****************************************************************
**igoto-viewer-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Unit tests for the igoto-viewer module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/igoto-viewer.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/igoto-viewer.hpp"

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
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[igoto-viewer] travel_cost" ) {
  world w;
}

TEST_CASE( "[igoto-viewer] heuristic_cost" ) {
  MockIGotoMapViewer viewer;
  point src, dst;

  auto const f = [&] [[clang::noinline]] {
    return viewer.heuristic_cost( src, dst );
  };

  src = {};
  dst = {};
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 0 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 0 );

  src = { .x = -2, .y = 4 };
  dst = { .x = 5, .y = 8 };
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 7 * 3 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 7 * 1 );

  src = { .x = -2, .y = 4 };
  dst = { .x = 5, .y = 11 };
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 7 * 3 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 7 * 1 );

  src = { .x = -2, .y = 4 };
  dst = { .x = 5, .y = 12 };
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 8 * 3 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 8 * 1 );
}

TEST_CASE( "[igoto-viewer] is_sea_lane_launch_point" ) {
  world w;
}

} // namespace
} // namespace rn
