/****************************************************************
**continental-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Unit tests for the continental module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/continental.hpp"

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
TEST_CASE( "[continental] compute_continental_promotion" ) {
  world w;
}

TEST_CASE( "[continental] do_continental_promotion" ) {
  world w;
}

TEST_CASE( "[continental] continental_promotion_ui_seq" ) {
  world w;
}

} // namespace
} // namespace rn
