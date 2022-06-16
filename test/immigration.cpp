/****************************************************************
**immigration.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: Unit tests for the src/immigration.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/immigration.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"

// Revolution Now
#include "src/gs-terrain.hpp"
#include "src/gs-units.hpp"
#include "src/harbor-units.hpp"
#include "src/igui-mock.hpp"
#include "src/igui.hpp"
#include "src/map-square.hpp"
#include "src/map-updater.hpp"
#include "src/player.hpp"
#include "src/ustate.hpp"

// Rds
#include "old-world-state.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

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
  World() : Base() {}

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1_w );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[immigration] ask_player_to_choose_immigrant" ) {
  ImmigrationState immigration{
      .immigrants_pool = { e_unit_type::expert_farmer,
                           e_unit_type::veteran_soldier,
                           e_unit_type::seasoned_scout } };
  MockIGui gui;

  EXPECT_CALL(
      gui, choice( ChoiceConfig{
               .msg = "please select one",
               .options =
                   vector<ChoiceConfigOption>{
                       { .key          = "0",
                         .display_name = "Expert Farmer" },
                       { .key          = "1",
                         .display_name = "Veteran Soldier" },
                       { .key          = "2",
                         .display_name = "Seasoned Scout" } },
               .key_on_escape = nothing } ) )
      .returns( make_wait<string>( "1" ) );

  wait<int> w = ask_player_to_choose_immigrant(
      gui, immigration, "please select one" );
  REQUIRE( w.ready() );
  REQUIRE( *w == 1 );
}

TEST_CASE( "[immigration] compute_crosses (dutch)" ) {
  World world;
  world.add_player( e_nation::dutch );
  world.set_default_player( e_nation::dutch );
  world.create_default_map();

  UnitsState const& units_state = world.units();
  Player&           player      = world.default_player();
  REQUIRE( player.crosses == 0 );

  CrossesCalculation crosses, expected;

  SECTION( "default" ) {
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed     = 8 + 2 * ( 0 + 0 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "some units in new world" ) {
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed     = 8 + 2 * ( 2 + 0 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "one unit on dock, no production" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -2,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 1 + 1 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/0,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 0 );
  }

  SECTION( "one unit on dock" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -2,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 1 + 1 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 1 );
  }

  SECTION( "two units in cargo of harbor ship" ) {
    // In this section we're testing that units that are in the
    // cargo of ships in harbor get treated as being on the dock.
    // This is done to uphold the spirit of the original game,
    // which did not have a concept of units being inside the
    // cargo of a ship in either the harbor view or colony view.
    UnitId ship = world.add_unit_in_port( e_unit_type::caravel );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -4,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 2 + 3 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 0 );
  }

  SECTION( "two units in cargo of outbound ship" ) {
    // In this section we're testing that units that are in the
    // cargo of ships in harbor get treated as being on the dock.
    // This is done to uphold the spirit of the original game,
    // which did not have a concept of units being inside the
    // cargo of a ship in either the harbor view or colony view.
    UnitId ship = world.add_unit_in_port( e_unit_type::caravel );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    world.add_unit_in_cargo( e_unit_type::free_colonist, ship );
    world.ship_to_outbound( ship );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 0 + 3 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "units on dock and in new world" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    world.add_unit_in_port( e_unit_type::free_colonist );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -4,
        // Dock units are counted twice.
        .crosses_needed = 8 + 2 * ( 4 + 2 ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/8,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 4 );
  }
}

TEST_CASE( "[immigration] compute_crosses (english)" ) {
  World world;
  world.add_player( e_nation::english );
  world.set_default_player( e_nation::english );
  world.create_default_map();

  UnitsState const& units_state = world.units();
  Player&           player      = world.default_player();
  REQUIRE( player.crosses == 0 );

  CrossesCalculation crosses, expected;

  SECTION( "default" ) {
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed =
            int( std::lround( .6666 * ( 8 + 2 * ( 0 + 0 ) ) ) ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "some units in new world" ) {
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    world.add_unit_on_map( e_unit_type::free_colonist, Coord{} );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = 2,
        .crosses_needed =
            int( std::lround( .6666 * ( 8 + 2 * ( 2 + 0 ) ) ) ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/3,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 5 );
  }

  SECTION( "one unit on dock, no production" ) {
    world.add_unit_in_port( e_unit_type::free_colonist );
    crosses  = compute_crosses( units_state, player.nation );
    expected = {
        .dock_crosses_bonus = -2,
        // Dock units are counted twice.
        .crosses_needed =
            int( std::lround( .6666 * ( 8 + 2 * ( 1 + 1 ) ) ) ),
    };
    REQUIRE( crosses == expected );
    add_player_crosses( player,
                        /*total_colonies_crosses_production=*/0,
                        crosses.dock_crosses_bonus );
    REQUIRE( player.crosses == 0 );
  }
}

} // namespace
} // namespace rn
