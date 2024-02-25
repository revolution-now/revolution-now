/****************************************************************
**rand-enum-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Unit tests for the rand-enum module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand-enum.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"
#include "test/rds/testing.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {}
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand-enum] pick-one" ) {
  World W;

  W.rand().EXPECT__between_ints( 0, 2 ).returns( 1 );

  auto const res = pick_one<e_color>( W.rand() );
  static_assert(
      std::is_same_v<decltype( res ), e_color const> );

  REQUIRE( res == e_color::green );
}

} // namespace
} // namespace rn
