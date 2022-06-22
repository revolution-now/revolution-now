/****************************************************************
**colview-buildings.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Unit tests for the src/colview-buildings.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/colview-buildings.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/gs-colonies.hpp"

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
  World() : Base() { add_player( e_nation::dutch ); }

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
    build_map( std::move( tiles ), 3_w );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[colview-buildings] layout" ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( Coord( 1_x, 1_y ) );

  // TODO
  (void)colony;
}

} // namespace
} // namespace rn
