/****************************************************************
**game-end-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Unit tests for the game-end module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
// #include "src/game-end.hpp"

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
    static MapSquare const _ = make_ocean();
    static MapSquare const L = make_grassland();
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
TEST_CASE( "[game-end] do_keep_playing_after_winning" ) {
  world w;
}

TEST_CASE( "[game-end] do_keep_playing_after_timeout" ) {
  world w;
}

TEST_CASE( "[game-end] ask_keep_playing" ) {
  world w;
}

TEST_CASE( "[game-end] check_time_up" ) {
  world w;
}

TEST_CASE( "[game-end] check_for_ref_win" ) {
  world w;
}

TEST_CASE( "[game-end] check_for_ref_forfeight" ) {
  world w;
}

} // namespace
} // namespace rn
