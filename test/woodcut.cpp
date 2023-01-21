/****************************************************************
**woodcut.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-21.
*
* Description: Unit tests for the src/woodcut.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/woodcut.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "src/ss/player.rds.hpp"

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
  World() : Base() { add_default_player(); }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[woodcut] display_woodcut" ) {
  World W;

  EXPECT_CALL(
      W.gui(),
      message_box(
          "(woodcut): Discovery of the Fountain of Youth!" ) )
      .returns<monostate>();
  wait<> w = detail::display_woodcut(
      W.gui(), e_woodcut::discovered_fountain_of_youth );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
}

TEST_CASE( "[woodcut] display_woodcut_if_needed" ) {
  World     W;
  Player&   player = W.default_player();
  e_woodcut cut    = {};

  auto f = [&] {
    wait<> w = display_woodcut_if_needed( W.ts(), player, cut );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  cut = e_woodcut::colony_destroyed;
  EXPECT_CALL( W.gui(), display_woodcut( cut ) )
      .returns<monostate>();
  REQUIRE_FALSE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );

  cut = e_woodcut::meeting_fellow_europeans;
  EXPECT_CALL( W.gui(), display_woodcut( cut ) )
      .returns<monostate>();
  REQUIRE_FALSE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );
}

} // namespace
} // namespace rn
