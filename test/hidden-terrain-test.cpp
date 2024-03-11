/****************************************************************
**hidden-terrain-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-10.
*
* Description: Unit tests for the hidden-terrain module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
// #include "src/hidden-terrain.hpp"

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
TEST_CASE( "[hidden-terrain] anim_seq_for_hidden_terrain" ) {
  World W;
}

} // namespace
} // namespace rn
