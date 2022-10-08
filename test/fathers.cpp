/****************************************************************
**fathers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-07.
*
* Description: Unit tests for the src/fathers.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/fathers.hpp"

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
    create_default_map();
    add_default_player();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L, L, L };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[fathers] pick_founding_father_if_needed" ) {
  World W;
  // TODO
}

TEST_CASE( "[fathers] check_founding_fathers" ) {
  World W;
  // TODO
}

TEST_CASE( "[fathers] play_new_father_cut_scene" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
