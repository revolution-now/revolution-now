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
#include "gs-events.hpp"
#include "gs-players.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "igui-mock.hpp"
#include "ustate.hpp"

// Under test.
#include "src/lcr.hpp"

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
  square.lost_city_rumor = true;
  REQUIRE( has_lost_city_rumor( terrain_state, Coord{} ) );
}

TEST_CASE( "[test/lcr] small village, chief gift" ) {
  UnitsState             units_state;
  TerrainState           terrain_state;
  EventsState            events_state;
  PlayersState           players_state;
  NonRenderingMapUpdater map_updater( terrain_state );
  MockIGui               gui;

  // Set nation.
  e_nation const nation = e_nation::dutch;

  // Set players.
  set_players( players_state, { e_nation::dutch } );
  Player& player = player_for_nation( players_state, nation );
  REQUIRE( player.money() == 0 );

  // Create map.
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  MapSquare& square = terrain_state.mutable_world_map()[Coord{}];
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id = create_unit_on_map(
      units_state, map_updater, nation,
      UnitComposition::create(
          UnitType::create( e_unit_type::free_colonist ) ),
      Coord{} );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::chief_gift;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.

  // Mock function calls.
  EXPECT_CALL( gui, message_box( StrContains(
                        "You happen upon a small village" ) ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult_t> lcr_res =
      run_lost_city_rumor_result(
          terrain_state, units_state, events_state, gui, player,
          map_updater, unit_id, /*move_dst=*/Coord{}, rumor_type,
          burial_type );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a chief gift to a non-scout on the lowest diffi-
  // culty mode.
  REQUIRE( player.money() >= 15 );
  REQUIRE( player.money() <= 70 );
}

} // namespace
} // namespace rn
