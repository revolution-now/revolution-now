/****************************************************************
**tax.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Unit tests for the src/tax.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/tax.hpp"

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
    add_default_player();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[tax] compute_tax_change" ) {
  World W;
  // TODO
}

TEST_CASE( "[tax] prompt_for_tax_change_result" ) {
  World W;
  // TODO
}

TEST_CASE( "[tax] apply_tax_result" ) {
  World W;
  // TODO
}

TEST_CASE( "[tax] start_of_turn_tax_check" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
