/****************************************************************
**raid.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-04-17.
*
* Description: Unit tests for the src/raid.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/raid.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/ref.hpp"

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
TEST_CASE( "[raid] perform_brave_attack_colony_effect" ) {
  World W;
}

TEST_CASE( "[raid] display_brave_attack_colony_effect_msg" ) {
  World W;
}

TEST_CASE( "[raid] select_brave_attack_colony_effect" ) {
  World W;
}

} // namespace
} // namespace rn
