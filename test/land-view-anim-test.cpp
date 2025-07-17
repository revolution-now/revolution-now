/****************************************************************
**land-view-anim-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-13.
*
* Description: Unit tests for the land-view-anim module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/land-view-anim.hpp"

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
  [[maybe_unused]] world() {
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
TEST_CASE( "[land-view-anim] AnimSeqOptions default values" ) {
  AnimSeqOptions opts;

  REQUIRE( opts.hold == false );
  // It is critical that this be true by default otherwise the
  // player will see literally ever possible animation of every
  // unit action.
  REQUIRE( opts.check_visibility == true );
}

// This test should check that the animate_sequence method sup-
// presses the animation if should_animate_seq returns false.
TEST_CASE( "[land-view-anim] animate_sequence checks visibility" ) {
  world w;
  // TODO
}

} // namespace
} // namespace rn
