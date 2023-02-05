/****************************************************************
**combat-modifiers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-04.
*
* Description: Unit tests for the src/combat-modifiers.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/combat-modifiers.hpp"

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

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[combat-modifiers] some test" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
