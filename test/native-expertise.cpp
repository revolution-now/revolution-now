/****************************************************************
**native-expertise.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-12.
*
* Description: Unit tests for the src/native-expertise.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/native-expertise.hpp"

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
    add_player( e_nation::english );
    set_default_player( e_nation::english );
    create_default_map();
  }

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
TEST_CASE( "[native-expertise] select_expertise_for_dwelling" ) {
  World W;
  // TODO
}

TEST_CASE( "[native-expertise] dwelling_expertise_weights" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
