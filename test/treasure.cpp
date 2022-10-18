/****************************************************************
**treasure.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-17.
*
* Description: Unit tests for the src/treasure.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/treasure.hpp"

// Testing
#include "test/fake/world.hpp"

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

  Coord const kColonySquare = Coord{ .x = 1, .y = 1 };

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[treasure] treasure_in_harbor_receipt" ) {
  World W;
  // TODO
}

TEST_CASE( "[treasure] treasure_enter_colony" ) {
  World W;
  // TODO
}

TEST_CASE( "[treasure] apply_treasure_reimbursement" ) {
  World W;
  // TODO
}

TEST_CASE( "[treasure] show_treasure_receipt" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
