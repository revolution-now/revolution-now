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
#include "src/harbor-units.hpp"
#include "src/igui-mock.hpp"
#include "src/igui.hpp"
#include "src/map-square.hpp"
#include "src/ustate.hpp"

// gs
#include "src/gs/player.rds.hpp"
#include "src/gs/settings.hpp"
#include "src/gs/terrain.hpp"
#include "src/gs/units.hpp"

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

using ::mock::matchers::AllOf;
using ::mock::matchers::Field;
using ::mock::matchers::HasSize;
using ::mock::matchers::StartsWith;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {}

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
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

TEST_CASE( "[immigration] take_immigrant_from_pool" ) {
  ImmigrationState state;
  state.immigrants_pool[0] = e_unit_type::expert_tobacco_planter;
  state.immigrants_pool[1] = e_unit_type::free_colonist;
  state.immigrants_pool[2] = e_unit_type::veteran_dragoon;

  take_immigrant_from_pool( state, 1,
                            e_unit_type::petty_criminal );

  REQUIRE( state.immigrants_pool[0] ==
           e_unit_type::expert_tobacco_planter );
  REQUIRE( state.immigrants_pool[1] ==
           e_unit_type::petty_criminal );
  REQUIRE( state.immigrants_pool[2] ==
           e_unit_type::veteran_dragoon );
}

// This is non-deterministic, but it should always pass when
// things are working as expected.
TEST_CASE( "[immigration] pick_next_unit_for_pool" ) {
  SettingsState settings;

  Player player;
  player.fathers.has[e_founding_father::william_brewster] = true;

  for( int difficulty = 0; difficulty < 5; ++difficulty ) {
    settings.difficulty = difficulty;
    for( int i = 0; i < 50; ++i ) {
      e_unit_type type =
          pick_next_unit_for_pool( player, settings );
      REQUIRE( type != e_unit_type::petty_criminal );
      REQUIRE( type != e_unit_type::indentured_servant );
    }
  }
}

TEST_CASE( "[immigration] check_for_new_immigrant" ) {
  MockIGui      gui;
  UnitsState    units_state;
  Player        player;
  SettingsState settings;

  // Set up the immigrants pool.
  ImmigrationState const initial_state = {
      .immigrants_pool = { e_unit_type::expert_farmer,
                           e_unit_type::veteran_soldier,
                           e_unit_type::seasoned_scout } };
  player.old_world.immigration.immigrants_pool =
      initial_state.immigrants_pool;

  SECTION( "not enough crosses" ) {
    player.crosses                     = 10;
    int const           crosses_needed = 11;
    wait<maybe<UnitId>> w              = check_for_new_immigrant(
                     gui, units_state, player, settings, crosses_needed );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( player.crosses == 10 );
    REQUIRE( units_state.all().size() == 0 );
    REQUIRE( player.old_world.immigration.immigrants_pool ==
             initial_state.immigrants_pool );
  }

  SECTION( "just enough crosses, no brewster" ) {
    player.crosses           = 11;
    int const crosses_needed = 11;

    EXPECT_CALL( gui, message_box( StartsWith(
                          "Word of religious freedom has "
                          "spread! A new immigrant" ) ) )
        .returns( make_wait<>() );
    wait<maybe<UnitId>> w = check_for_new_immigrant(
        gui, units_state, player, settings, crosses_needed );
    REQUIRE( w.ready() );
    REQUIRE( *w == UnitId{ 1 } );

    REQUIRE( player.crosses == 0 );
    REQUIRE( units_state.all().size() == 1 );
    REQUIRE( units_state.all().begin()->first == UnitId{ 1 } );
    UnitOwnership_t const expected_ownership{
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{} } } };
    REQUIRE( units_state.all().begin()->second.ownership ==
             expected_ownership );
  }

  SECTION( "enough crosses, with brewster" ) {
    player.crosses = 13;
    player.fathers.has[e_founding_father::william_brewster] =
        true;
    int const crosses_needed = 11;

    EXPECT_CALL(
        gui,
        choice( AllOf(
            Field( &ChoiceConfig::msg,
                   StartsWith(
                       "Word of religious freedom has spread! "
                       "New immigrants are ready to join us in "
                       "the New World.  Which of the following "
                       "shall we choose?" ) ),
            Field( &ChoiceConfig::options, HasSize( 3 ) ),
            Field( &ChoiceConfig::key_on_escape,
                   maybe<string>{} ) ) ) )
        .returns( make_wait<string>( "1" ) );
    wait<maybe<UnitId>> w = check_for_new_immigrant(
        gui, units_state, player, settings, crosses_needed );
    REQUIRE( w.ready() );
    REQUIRE( *w == UnitId{ 1 } );

    REQUIRE( player.crosses == 2 );
    REQUIRE( units_state.all().size() == 1 );
    REQUIRE( units_state.all().begin()->first == UnitId{ 1 } );
    UnitOwnership_t const expected_ownership{
        UnitOwnership::harbor{
            .st = UnitHarborViewState{
                .port_status = PortStatus::in_port{} } } };
    REQUIRE( units_state.all().begin()->second.ownership ==
             expected_ownership );
  }
}

} // namespace
} // namespace rn
