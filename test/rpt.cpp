/****************************************************************
**rpt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Unit tests for the src/test/rpt.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rpt.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "ss/player.hpp"

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
  World() : Base() { add_player( e_nation::english ); }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rpt] some test" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
