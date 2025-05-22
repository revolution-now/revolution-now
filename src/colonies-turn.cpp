/****************************************************************
**colonies-turn.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-02.
*
* Description: Top-level colony evolution logic run each turn.
*
*****************************************************************/
#include "colonies-turn.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "colony-view.rds.hpp"
#include "damaged.rds.hpp"
#include "harbor-units.hpp"
#include "harbor-view.hpp"
#include "icolony-evolve.rds.hpp"
#include "igui.hpp"
#include "immigration.hpp"
#include "land-view.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/nation.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

// C++ standard library
#include <numeric>
#include <unordered_map>
#include <vector>

using namespace std;

namespace rn {

namespace {

// Returns true if the user wants to open the colony view.
wait<bool> present_blocking_colony_update(
    IGui& gui, ColonyNotificationMessage const& msg,
    bool ask_to_zoom ) {
  CHECK( !msg.transient );
  if( ask_to_zoom ) {
    vector<ChoiceConfigOption> choices{
      { .key = "no_zoom", .display_name = "Continue turn" },
      { .key = "zoom", .display_name = "Zoom to colony" } };
    maybe<string> res = co_await gui.optional_choice(
        { .msg = msg.msg, .options = std::move( choices ) } );
    // If the user hits escape then we don't zoom.
    co_return ( res == "zoom" );
  }
  co_await gui.message_box( msg.msg );
  co_return false;
}

// The idea here (taken from the original game) is that the first
// message will always ask the player if they want to open the
// colony view. If they select no, then any subsequent messages
// will continue to ask them until they select yes. Once they se-
// lect yes (if they ever do) then subsequent messages will still
// be displayed but will not ask them.
wait<bool> present_blocking_colony_updates(
    IGui& gui,
    vector<ColonyNotificationMessage> const& messages ) {
  bool should_zoom = false;
  for( ColonyNotificationMessage const& message : messages ) {
    bool const wants_zoom =
        co_await present_blocking_colony_update( gui, message,
                                                 !should_zoom );
    should_zoom = should_zoom || wants_zoom;
  }
  co_return should_zoom;
}

// These are messages from all colonies that are to appear in the
// transient pop-up (i.e., the window that is non-blocking, takes
// no input, and fades away on its own).
void present_transient_updates(
    TS& ts, vector<ColonyNotificationMessage> const& messages ) {
  for( ColonyNotificationMessage const& msg : messages ) {
    CHECK( msg.transient );
    ts.gui.transient_message_box( msg.msg );
  }
}

void give_new_crosses_to_player(
    Player& player, CrossesCalculation const& crosses_calc,
    vector<ColonyEvolution> const& evolutions ) {
  int const colonies_crosses =
      accumulate( evolutions.begin(), evolutions.end(), 0,
                  []( int so_far, ColonyEvolution const& ev ) {
                    return so_far + ev.production.crosses;
                  } );
  add_player_crosses( player, colonies_crosses,
                      crosses_calc.dock_crosses_bonus );
}

wait<> run_colony_starvation( SS& ss, TS& ts, Colony& colony ) {
  // Must extract this info before destroying the colony.
  string const msg = fmt::format(
      "[{}] ran out of food and was not able to support "
      "its last remaining colonists.  As a result, the colony "
      "has disappeared.",
      colony.name );
  co_await run_animated_colony_destruction(
      ss, ts, colony, e_ship_damaged_reason::colony_starved,
      msg );
  // !! Do not reference `colony` beyond this point.
}

wait<> fire_fortifications( SS&, TS&, Player&, Colony& ) {
  // TODO: Forts and Fortresses fire on units that are adjacent
  // to the fortress. The same fort/fortress can fire as many
  // times as there are units adjacent to it. So just iterate
  // over all of the surrounding squares in clockwise order and
  // do one at a time. NOTE: it seems that in some cases the ar-
  // tillery icon has a red X over it; one thought was that this
  // happens when the colony has no artillery units in it, but it
  // doesn't seem to happen consistently in relation to that, so
  // not sure what it is.
  co_return;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> evolve_colonies_for_player(
    SS& ss, TS& ts, Player& player,
    IColonyEvolver const& colony_evolver,
    IHarborViewer& harbor_viewer,
    IColonyNotificationGenerator const&
        colony_notification_generator ) {
  e_player const player_type = player.type;
  lg.info( "processing colonies for the {}.", player_type );
  unordered_map<ColonyId, Colony> const& colonies_all =
      ss.colonies.all();
  vector<ColonyId> colonies;
  colonies.reserve( colonies_all.size() );
  for( auto const& [colony_id, colony] : colonies_all )
    if( colony.player == player_type )
      colonies.push_back( colony_id );
  // This is so that we process them in a deterministic order
  // that doesn't depend on hash map iteration order.
  sort( colonies.begin(), colonies.end() );
  vector<ColonyEvolution> evolutions;
  // These will be accumulated for all colonies and then dis-
  // played at the end.
  vector<ColonyNotificationMessage> transient_messages;
  for( ColonyId const colony_id : colonies ) {
    Colony& colony = ss.colonies.colony_for( colony_id );
    lg.debug( "evolving colony \"{}\".", colony.name );
    // The OG appears to do this precisely here, namely at the
    // start of each colony's evolution, but before evolving the
    // colony and showing colony notifications.
    co_await fire_fortifications( ss, ts, player, colony );
    evolutions.push_back(
        colony_evolver.evolve_colony_one_turn( colony ) );
    ColonyEvolution const& ev = evolutions.back();
    if( ev.colony_disappeared ) {
      co_await run_colony_starvation( ss, ts, colony );
      // !! at this point the colony will have been deleted, so
      // we should not access it anymore.
      continue;
    }
    if( ev.notifications.empty() ) continue;
    // Separate the transient messages from the blocking mes-
    // sages.
    vector<ColonyNotificationMessage> blocking_messages;
    blocking_messages.reserve( ev.notifications.size() );
    for( ColonyNotification const& notification :
         ev.notifications ) {
      ColonyNotificationMessage msg =
          colony_notification_generator
              .generate_colony_notification_message(
                  colony, notification );
      if( msg.transient )
        transient_messages.push_back( std::move( msg ) );
      else
        blocking_messages.push_back( std::move( msg ) );
    }
    if( !blocking_messages.empty() )
      // We have some blocking notifications to present.
      co_await ts.planes.get()
          .get_bottom<ILandViewPlane>()
          .ensure_visible( colony.location );
    bool const zoom_to_colony =
        co_await present_blocking_colony_updates(
            ts.gui, blocking_messages );
    if( zoom_to_colony ) {
      e_colony_abandoned abandoned =
          co_await ts.colony_viewer.show( ts, colony.id );
      if( abandoned == e_colony_abandoned::yes ) continue;
    }
  }

  // Now that all colonies are done, present all of the transient
  // messages.
  present_transient_updates( ts, transient_messages );

  // Crosses/immigration.
  CrossesCalculation const crosses_calc =
      compute_crosses( ss.units, player.type );
  give_new_crosses_to_player( player, crosses_calc, evolutions );
  maybe<UnitId> immigrant = co_await check_for_new_immigrant(
      ss, ts, player, crosses_calc.crosses_needed );
  if( immigrant.has_value() ) {
    lg.info( "a new immigrant ({}) has arrived.",
             ss.units.unit_for( *immigrant ).desc().name );
    // When a new colonist arrives on the dock, the OG shows the
    // harbor view just after displaying the message but only
    // when there is a ship in the harbor.
    int const num_ships_in_port =
        harbor_units_in_port( ss.units, player.type ).size();
    if( num_ships_in_port > 0 ) co_await harbor_viewer.show();
  }
}

} // namespace rn
