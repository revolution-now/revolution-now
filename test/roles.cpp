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
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    // !! Do not create players here.
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[roles] player_for_role" ) {
  World W;

  auto f = [&]( e_player_role role ) {
    return player_for_role( W.ss(), role );
  };

  W.land_view().map_revealed = MapRevealed::no_special_view{};

  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Add french player.
  W.add_player( e_nation::french );
  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Add dutch player.
  W.add_player( e_nation::dutch );
  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Make dutch human.
  W.players().human = e_nation::dutch;
  REQUIRE( f( e_player_role::viewer ) == e_nation::dutch );
  REQUIRE( f( e_player_role::human ) == e_nation::dutch );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Add spanish player.
  W.add_player( e_nation::spanish );
  REQUIRE( f( e_player_role::viewer ) == e_nation::dutch );
  REQUIRE( f( e_player_role::human ) == e_nation::dutch );
  REQUIRE( f( e_player_role::active ) == nothing );

  // Make spanish active.
  W.turn().cycle =
      TurnCycle::nation{ .nation = e_nation::spanish };
  REQUIRE( f( e_player_role::viewer ) == e_nation::dutch );
  REQUIRE( f( e_player_role::human ) == e_nation::dutch );
  REQUIRE( f( e_player_role::active ) == e_nation::spanish );

  // Remove human status.
  W.players().human = nothing;
  REQUIRE( f( e_player_role::viewer ) == e_nation::spanish );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == e_nation::spanish );

  // Add english player.
  W.add_player( e_nation::english );
  REQUIRE( f( e_player_role::viewer ) == e_nation::spanish );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == e_nation::spanish );

  // Switch to entire-map-view.
  W.land_view().map_revealed = MapRevealed::entire{};
  REQUIRE( f( e_player_role::viewer ) == nothing );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == e_nation::spanish );

  // Switch to english nation-map-view.
  W.land_view().map_revealed =
      MapRevealed::nation{ .nation = e_nation::english };
  REQUIRE( f( e_player_role::viewer ) == e_nation::english );
  REQUIRE( f( e_player_role::human ) == nothing );
  REQUIRE( f( e_player_role::active ) == e_nation::spanish );

  // Make french human.
  W.players().human = e_nation::french;
  REQUIRE( f( e_player_role::viewer ) == e_nation::english );
  REQUIRE( f( e_player_role::human ) == e_nation::french );
  REQUIRE( f( e_player_role::active ) == e_nation::spanish );
}

} // namespace
} // namespace rn
