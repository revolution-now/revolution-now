/****************************************************************
**mv-calc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-28.
*
* Description: Unit tests for the src/mv-calc.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/mv-calc.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/ustate.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::Approx;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
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

  inline static Coord const kLand = Coord{ .x = 1, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[src/mv-calc] can_unit_move_based_on_mv_points" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
