/****************************************************************
**visibility.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-30.
*
* Description: Unit tests for the src/visibility.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/visibility.hpp"

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
    create_default_map();
    set_default_player( e_nation::french );
    add_player( e_nation::french );
    add_player( e_nation::english );
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
  }

  void create_default_map() {
    MapSquare const L = make_grassland();
    MapSquare const _ = make_ocean();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, _, L, _, L, L, L, L, L, L, _, L, L,
      L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, _, L, L, L, L, L, L, L, L, L,
      _, L, _, _, L, _, L, _, L, L, L, L, _, L, L,
      L, L, L, L, L, L, L, L, _, L, L, L, L, L, L,
      _, L, L, L, L, _, _, _, _, _, L, L, L, L, L,
      _, L, _, _, L, _, L, _, _, _, _, L, _, L, L,
      L, L, L, L, L, L, L, _, _, _, _, L, L, L, L,
      _, L, L, L, L, _, L, L, L, _, L, L, L, L, L,
      _, L, _, _, L, _, L, _, L, L, L, L, _, L, L,
      L, L, L, L, L, L, L, _, _, L, L, L, L, L, L,
      _, L, L, L, L, _, L, L, L, L, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 15 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[visibility] nations_with_visibility_of_square" ) {
  World W;
  // TODO
}

TEST_CASE( "[visibility] unit_sight_radius" ) {
  World W;
  // TODO
}

TEST_CASE( "[visibility] unit_visible_squares" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
