/****************************************************************
**lcr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-23.
*
* Description: Unit tests for the src/lcr.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Revolution Now
#include "gs-players.hpp"
#include "gs-settings.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "igui-mock.hpp"
#include "ustate.hpp"

// Under test.
#include "src/lcr.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/keyval.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::mock::matchers;

TEST_CASE( "[test/lcr] has_lost_city_rumor" ) {
  TerrainState terrain_state;
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );

  REQUIRE_FALSE( has_lost_city_rumor( terrain_state, Coord{} ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;
  REQUIRE( has_lost_city_rumor( terrain_state, Coord{} ) );
}

TEST_CASE( "[test/lcr] de soto means no unit lost" ) {
  PlayersState   players_state;
  e_nation const nation = e_nation::dutch;
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  player.fathers.has[e_founding_father::hernando_de_soto] = true;
  for( int i = 0; i < 100; ++i ) {
    INFO( fmt::format( "i: {}", i ) );
    e_rumor_type type = pick_rumor_type_result(
        e_lcr_explorer_category::other, player );
    REQUIRE( type != e_rumor_type::unit_lost );
  }
}

TEST_CASE( "[test/lcr] nothing but rumors" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::none;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL(
      gui, message_box( StrContains( "nothing but rumors" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

TEST_CASE( "[test/lcr] small village, chief gift" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::chief_gift;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL( gui, message_box( StrContains(
                        "You happen upon a small village" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a chief gift to a non-scout on the lowest diffi-
  // culty mode.
  REQUIRE( player.money >= 15 );
  REQUIRE( player.money <= 70 );
  REQUIRE( player.money % 1 == 0 ); // trivial
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

TEST_CASE( "[test/lcr] small village, ruins of lost colony" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::ruins;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL(
      gui,
      message_box( StrContains( "ruins of a lost colony" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a ruins of lost colony to a non-scout on the
  // lowest difficulty mode.
  REQUIRE( player.money >= 80 );
  REQUIRE( player.money <= 220 );
  REQUIRE( player.money % 10 == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

TEST_CASE( "[test/lcr] fountain of youth" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type rumor_type = e_rumor_type::fountain_of_youth;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL( gui,
               message_box( StrContains(
                   "You've discovered a Fountain of Youth!" ) ) )
      .returns( make_wait() );
  // Need to do this in a loop because we need a separate return
  // object for each one (since they are moved).
  for( int i = 0; i < 8; ++i ) {
    EXPECT_CALL(
        gui, choice( Field(
                 &ChoiceConfig::msg,
                 StrContains( "Who shall we next choose" ) ) ) )
        .returns( make_wait<string>( "1" ) );
    EXPECT_CALL( gui, wait_for( chrono::milliseconds( 300 ) ) )
        .returns( chrono::microseconds{} );
  }

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 9 );
}

TEST_CASE( "[test/lcr] free colonist" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::free_colonist;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL(
      gui,
      message_box( StrContains( "happen upon the survivors" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorResult::unit_created>().id ==
      2_id );
  REQUIRE( player.money == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 2 );
}

TEST_CASE( "[test/lcr] unit lost" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::unit_lost;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL(
      gui,
      message_box( StrContains( "vanished without a trace" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_lost>() );
  REQUIRE( player.money == 0 );
  REQUIRE_FALSE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 0 );
}

TEST_CASE( "[test/lcr] burial mounds / treasure" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::treasure_train;
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL( gui, choice( _ ) )
      .returns( make_wait<string>( "yes" ) );
  EXPECT_CALL( gui, message_box( StrContains(
                        "recovered a treasure worth" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorResult::unit_created>().id ==
      2_id );
  Unit const& unit = units_state.unit_for( 2_id );
  REQUIRE( unit.type() == e_unit_type::large_treasure );
  unordered_map<e_unit_inventory, int> const& inventory =
      unit.composition().inventory();
  REQUIRE( inventory.contains( e_unit_inventory::gold ) );
  UNWRAP_CHECK(
      gold, base::lookup( inventory, e_unit_inventory::gold ) );
  // These number come from the config files for the min/max
  // amount of a treasure train for a non-scout on the lowest
  // difficulty mode.
  REQUIRE( gold >= 2000 );
  REQUIRE( gold <= 3500 );
  REQUIRE( gold % 100 == 0 );
  // Money is zero because the gold is on the treasure train.
  REQUIRE( player.money == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 2 );
}

TEST_CASE( "[test/lcr] burial mounds / cold and empty" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty;
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL( gui, choice( _ ) )
      .returns( make_wait<string>( "yes" ) );
  EXPECT_CALL( gui,
               message_box( StrContains( "cold and empty" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

TEST_CASE( "[test/lcr] burial mounds / trinkets" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::trinkets;
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL( gui, choice( _ ) )
      .returns( make_wait<string>( "yes" ) );
  EXPECT_CALL(
      gui, message_box( StrContains( "found some trinkets" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money >= 70 );
  REQUIRE( player.money <= 200 );
  REQUIRE( player.money % 10 == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

TEST_CASE( "[test/lcr] burial mounds / no explore" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::trinkets;
  bool has_burial_grounds = false;

  // Mock function calls.
  EXPECT_CALL( gui, choice( _ ) )
      .returns( make_wait<string>( "no" ) );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

TEST_CASE(
    "[test/lcr] burial mounds / trinkets with burial grounds" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  UnitsState             units_state;
  TerrainState           terrain_state;
  PlayersState           players_state;
  SettingsState          settings;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  reset_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map_non_interactive(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );
  REQUIRE( units_state.all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::trinkets;
  bool has_burial_grounds = true;

  // Mock function calls.
  EXPECT_CALL( gui, choice( _ ) )
      .returns( make_wait<string>( "yes" ) );
  EXPECT_CALL(
      gui, message_box( StrContains( "found some trinkets" ) ) )
      .returns( make_wait() );
  EXPECT_CALL(
      gui,
      message_box( StrContains( "native burial grounds" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result( units_state, gui, player,
                                  settings, map_updater, unit_id,
                                  /*move_dst=*/Coord{},
                                  rumor_type, burial_type,
                                  has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money >= 70 );
  REQUIRE( player.money <= 200 );
  REQUIRE( player.money % 10 == 0 );
  REQUIRE( units_state.exists( unit_id ) );
  REQUIRE( units_state.all().size() == 1 );
}

} // namespace
} // namespace rn
