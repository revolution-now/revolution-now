/****************************************************************
**game-setup-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Unit tests for the game-setup module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/game-setup.hpp"

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
    using enum e_player;
    add_player( english );
    set_default_player_type( english );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const S = make_ocean();
    static MapSquare const _ = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{/*
          0 1 2 3 4 5 6 7
      0*/ S,_,_,_,_,_,_,S, /*0
      1*/ S,_,_,_,_,_,_,S, /*1
      2*/ S,_,_,_,_,_,_,S, /*2
      3*/ S,_,_,_,_,_,_,S, /*3
      4*/ S,_,_,_,_,_,_,S, /*4
      5*/ S,_,_,_,_,_,_,S, /*5
      6*/ S,_,_,_,_,_,_,S, /*6
      7*/ S,_,_,_,_,_,_,S, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[game-setup] create_default_game_setup" ) {
  world w;
}

TEST_CASE( "[game-setup] create_america_game_setup" ) {
  world w;
}

TEST_CASE( "[game-setup] create_customized_game_setup" ) {
  world w;
}

} // namespace
} // namespace rn
