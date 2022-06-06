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
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/immigration.hpp"

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
** World Setup
*****************************************************************/
Coord const kSquare( 0_x, 0_y );

// This will prepare a world with a 1x1 map.
void prepare_world( TerrainState& terrain_state ) {
  NonRenderingMapUpdater map_updater( terrain_state );
  map_updater.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m          = Matrix<MapSquare>( Delta( 1_w, 1_h ) );
    m[kSquare] = map_square_for_terrain( e_terrain::grassland );
  } );
}

UnitId add_unit_to_new_world( UnitsState&   units_state,
                              TerrainState& terrain_state,
                              e_nation      nation ) {
  UnitId                 id = create_unit( units_state, nation,
                                           e_unit_type::free_colonist );
  NonRenderingMapUpdater map_updater( terrain_state );
  unit_to_map_square_non_interactive( units_state, map_updater,
                                      id, kSquare );
  return id;
}

UnitId add_unit_to_dock( UnitsState& units_state,
                         e_nation    nation ) {
  UnitId id = create_unit( units_state, nation,
                           e_unit_type::free_colonist );
  unit_move_to_port( units_state, id );
  return id;
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[immigration] ask_player_to_choose_immigrant" ) {
  ImmigrationState immigration{
      .immigrants_pool =
          {
              e_unit_type::expert_farmer,
              e_unit_type::veteran_soldier,
              e_unit_type::seasoned_scout,
          },
  };
  MockIGui gui;

  EXPECT_CALL( gui,
               choice( ChoiceConfig{
                   .msg = "please select one",
                   .options =
                       {
                           { .key          = "0",
                             .display_name = "Expert Farmer" },
                           { .key          = "1",
                             .display_name = "Veteran Soldier" },
                           { .key          = "2",
                             .display_name = "Seasoned Scout" },
                       },
                   .key_on_escape = nothing,
               } ) )
      .returns( make_wait<string>( "1" ) );

  wait<int> w = ask_player_to_choose_immigrant(
      gui, immigration, "please select one" );
  REQUIRE( w.ready() );
  REQUIRE( *w == 1 );
}

TEST_CASE( "[immigration] compute_crosses (dutch)" ) {
  UnitsState   units_state;
  TerrainState terrain_state;
  Player       player;
  player.nation = e_nation::dutch;
  REQUIRE( player.crosses == 0 );

  CrossesCalculation crosses, expected;

  prepare_world( terrain_state );

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
    add_unit_to_new_world( units_state, terrain_state,
                           player.nation );
    add_unit_to_new_world( units_state, terrain_state,
                           player.nation );
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
    add_unit_to_dock( units_state, player.nation );
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
    add_unit_to_dock( units_state, player.nation );
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

  SECTION( "units on dock and in new world" ) {
    add_unit_to_dock( units_state, player.nation );
    add_unit_to_dock( units_state, player.nation );
    add_unit_to_new_world( units_state, terrain_state,
                           player.nation );
    add_unit_to_new_world( units_state, terrain_state,
                           player.nation );
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
  UnitsState   units_state;
  TerrainState terrain_state;
  Player       player;
  player.nation = e_nation::english;
  REQUIRE( player.crosses == 0 );

  CrossesCalculation crosses, expected;

  prepare_world( terrain_state );

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
    add_unit_to_new_world( units_state, terrain_state,
                           player.nation );
    add_unit_to_new_world( units_state, terrain_state,
                           player.nation );
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
    add_unit_to_dock( units_state, player.nation );
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
