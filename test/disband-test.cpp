/****************************************************************
**disband-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-10.
*
* Description: Unit tests for the disband module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/disband.hpp"

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
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[disband] disbandable_entities_on_tile" ) {
  world w;
}

TEST_CASE( "[disband] disband_tile_ui_interaction" ) {
  world w;
}

TEST_CASE( "[disband] execute_disband" ) {
  world w;
}

} // namespace
} // namespace rn
