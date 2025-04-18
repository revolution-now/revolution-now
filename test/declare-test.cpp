/****************************************************************
**declare-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-13.
*
* Description: Unit tests for the declare module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/declare.hpp"

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
TEST_CASE( "[declare] can_declare_independence" ) {
  world w;
}

TEST_CASE( "[declare] show_declare_rejection_msg" ) {
  world w;
}

TEST_CASE( "[declare] ask_declare" ) {
  world w;
}

TEST_CASE( "[declare] player_that_declared" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence_ui_sequence_pre" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence" ) {
  world w;
}

TEST_CASE( "[declare] declare_independence_ui_sequence_post" ) {
  world w;
}

} // namespace
} // namespace rn
