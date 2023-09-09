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

  W.gui()
      .EXPECT__message_box(
          "(woodcut): Discovery of the Fountain of Youth!" )
      .returns();
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
    wait<> w = display_woodcut_if_needed( W.gui(), player, cut );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  cut = e_woodcut::colony_destroyed;
  W.gui().EXPECT__display_woodcut( cut ).returns();
  REQUIRE_FALSE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );

  cut = e_woodcut::meeting_fellow_europeans;
  W.gui().EXPECT__display_woodcut( cut ).returns();
  REQUIRE_FALSE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );
  f();
  REQUIRE( player.woodcuts[cut] );
}

} // namespace
} // namespace rn
