/****************************************************************
**autosave-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-06.
*
* Description: Unit tests for the autosave module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/autosave.hpp"

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
  World() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, L, //
        L, L, L, //
        L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[autosave] should_autosave" ) {
  World w;
}

TEST_CASE( "[autosave] autosave" ) {
  World w;
}

} // namespace
} // namespace rn
