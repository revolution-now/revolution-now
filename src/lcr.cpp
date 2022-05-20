/****************************************************************
**lcr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-19.
*
* Description: All things related to Lost City Rumors.
*
*****************************************************************/
#include "lcr.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "gs-events.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "window.hpp"

using namespace std;

namespace rn {

namespace {

enum class e_burial_mounds_type {
  // Trinkets (small amount of gold).
  trinkets,
  // Treasure (creates treasure train unit).
  // TODO: Seven cities of Cibola? El Dorado?
  treasure_train,
  // Nothing ("mounds are cold and empty").
  cold_and_empty,
};

enum class e_rumor_type {
  // "Nothing but rumors."
  none,
  // Fountain of Youth (eight immigrants at dock).
  fountain_of_youth,
  // Ruins of lost civ (small amount of gold).
  ruins,
  // Burial mounds (let us search for treasure/stay clear of
  // those). Expands to e_burial_mounds_type. This is not to be
  // confused with "burial grounds", although the latter can re-
  // sult (regardless of what is found in the mounds), if the
  // land is owned by indians.
  burial_mounds,
  // Small village, chief offers gift (small amount of money).
  chief_gift,
  // Survivors of lost colony; free_colonist is created.
  free_colonist,
  // Unit vanishes.
  unit_lost,
  // Learn about nearby lands. TODO: does not seem to happen.
  nearby_land,
  // Upgrade to seasoned scout (for scouts only). TODO: does not
  // seem to happen.
  scout_upgrade,
};

// When exploring burial mounds that are in native owned land we
// could find indian burial grounds, which will cause that tribe
// to declare war (permanently?).
bool has_burial_grounds() {
  // TODO
  return false;
}

// The fountain of youth is only allowed pre-independence.
bool allow_fountain_of_youth( EventsState const& events_state ) {
  return !events_state.independence_declared;
}

} // namespace

bool has_lost_city_rumor( TerrainState const& terrain_state,
                          Coord               square ) {
  return terrain_state.square_at( square ).lost_city_rumor;
}

wait<bool> enter_lost_city_rumor(
    TerrainState const& terrain_state, UnitsState& units_state,
    EventsState const& events_state, Player& player,
    IMapUpdater& map_updater, UnitId unit_id,
    Coord world_square ) {
  // TODO
  (void)has_burial_grounds();
  (void)allow_fountain_of_youth( events_state );

  co_await ui::message_box(
      "You've entered a lost city rumor!" );

  // Remove lost city rumor.
  map_updater.modify_map_square(
      world_square, []( MapSquare& square ) {
        CHECK_EQ( square.lost_city_rumor, true );
        square.lost_city_rumor = false;
      } );

  co_return false; // did not kill unit.
}

} // namespace rn
