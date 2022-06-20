/****************************************************************
**colony-evolve.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-06.
*
* Description: Unit tests for the src/colony-evolve.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/colony-evolve.hpp"

// Testing.
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
  World() : Base() { add_player( e_nation::dutch ); }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const S = make_sea_lane();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3_w );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[colony-evolve] applies production" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[colony-evolve] construction" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[colony-evolve] new colonist" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[colony-evolve] colonist starved" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[colony-evolve] spoilage" ) {
  World W;
  W.create_default_map();
  // TODO
}

} // namespace
} // namespace rn
