/****************************************************************
**roles.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-01.
*
* Description: Unit tests for the src/roles.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/roles.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/turn.rds.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    // !! Do not create players here.
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[roles] player_for_role (viewer+active)" ) {
  world w;

  auto const f = [&]( e_player_role const role ) {
    return player_for_role( w.ss(), role );
  };

  w.land_view().map_revealed = MapRevealed::no_special_view{};

  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Add french player.
  w.add_player( e_player::french );
  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Add dutch player.
  w.add_player( e_player::dutch );
  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Make dutch human.
  w.dutch().control = e_player_control::human;
  REQUIRE( f( e_player_role::viewer ) == e_player::dutch );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Add spanish player.
  w.add_player( e_player::spanish );
  REQUIRE( f( e_player_role::viewer ) == e_player::dutch );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Make spanish active.
  w.turn().cycle =
      TurnCycle::player{ .type = e_player::spanish };
  REQUIRE( f( e_player_role::viewer ) == e_player::dutch );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Make spanish human.
  w.spanish().control = e_player_control::human;
  REQUIRE( f( e_player_role::viewer ) == e_player::spanish );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Remove human status from spanish.
  w.spanish().control = e_player_control::ai;
  REQUIRE( f( e_player_role::viewer ) == e_player::dutch );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Remove human status from dutch.
  w.dutch().control = e_player_control::ai;
  REQUIRE( f( e_player_role::viewer ) == e_player::spanish );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Add english player.
  w.add_player( e_player::english );
  REQUIRE( f( e_player_role::viewer ) == e_player::spanish );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Switch to entire-map-view.
  w.land_view().map_revealed = MapRevealed::entire{};
  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Switch to english player-map-view.
  w.land_view().map_revealed =
      MapRevealed::player{ .type = e_player::english };
  REQUIRE( f( e_player_role::viewer ) == e_player::english );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Make french human.
  w.french().control = e_player_control::human;
  REQUIRE( f( e_player_role::viewer ) == e_player::english );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );

  // Switch to no special view.
  w.land_view().map_revealed = MapRevealed::no_special_view{};
  REQUIRE( f( e_player_role::viewer ) == e_player::french );
  REQUIRE( f( e_player_role::active ) == e_player::spanish );
}

TEST_CASE( "[roles] player_for_role (primary_human)" ) {
  world w;

  auto const f = [&] [[clang::noinline]] {
    return player_for_role( w.ss(),
                            e_player_role::primary_human );
  };

  REQUIRE( f() == nothing );
}

} // namespace
} // namespace rn
