/****************************************************************
**goto-viewer-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Unit tests for the goto-viewer module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
// #include "src/goto-viewer.hpp"

// Testing.
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

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
TEST_CASE( "[goto-viewer] can_enter_tile" ) {
  world w;
}

TEST_CASE( "[goto-viewer] map_side" ) {
  world w;
}

TEST_CASE( "[goto-viewer] is_on_map_side_edge" ) {
  world w;
}

TEST_CASE( "[goto-viewer] is_sea_lane" ) {
  world w;
}

TEST_CASE( "[goto-viewer] movement_points_required" ) {
  world w;
}

} // namespace
} // namespace rn
