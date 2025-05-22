/****************************************************************
**damaged.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-17.
*
* Description: Things related to damaged ships.
*
*****************************************************************/
#include "damaged.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "harbor-units.hpp"
#include "revolution-status.hpp"
#include "unit-ownership.hpp"

// config
#include "config/nation.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// Rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/conv.hpp"
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

struct ColonyWithDrydock {
  ColonyId colony_id = {};
  // Pythagorean distance from the ship to the colony.
  double distance = {};
};

ShipRepairPort find_repair_port_for_ship_pre_independence(
    SSConst const& ss, e_player player, Coord ship_location ) {
  vector<ColonyId> colonies = ss.colonies.for_player( player );
  // So that we don't rely on hash map iteration order.
  sort( colonies.begin(), colonies.end() );
  if( colonies.empty() )
    return ShipRepairPort::european_harbor{};
  maybe<ColonyWithDrydock> found_colony;
  for( ColonyId const colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    if( !colony_has_building_level(
            colony, e_colony_building::drydock ) )
      continue;
    // Note: we don't need to check if the colony has ocean ac-
    // cess here because the game (like OG) will not allow the
    // construction ofa Drydock or Shipyard in a colony without
    // ocean access. Though also, like the OG, if we inject a
    // Drydock into the colony via cheat mode, then we may end up
    // sending the ship there, but so be it.
    double const distance =
        ( colony.location - ship_location ).diagonal();
    if( !found_colony.has_value() ||
        distance < found_colony->distance )
      found_colony = ColonyWithDrydock{ .colony_id = colony_id,
                                        .distance  = distance };
  }
  if( !found_colony.has_value() )
    return ShipRepairPort::european_harbor{};
  return ShipRepairPort::colony{ .id = found_colony->colony_id };
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<ShipRepairPort> find_repair_port_for_ship(
    SSConst const& ss, e_player const player_type,
    Coord ship_location ) {
  ShipRepairPort const res =
      find_repair_port_for_ship_pre_independence(
          ss, player_type, ship_location );
  Player const& player =
      player_for_player_or_die( ss.players, player_type );
  if( player.revolution.status >=
      e_revolution_status::declared ) {
    // After independence is declared we cannot go back to the
    // european harbor for any reason, including to repair ships.
    if( res.holds<ShipRepairPort::european_harbor>() )
      return nothing;
  }
  return res;
}

string ship_still_damaged_message( int turns_until_repaired ) {
  CHECK_GT( turns_until_repaired, 0 );
  string_view const s = ( turns_until_repaired > 1 ) ? "s" : "";
  return fmt::format(
      "This ship is [damaged] and has [{}] turn{} remaining "
      "until it is repaired.",
      base::int_to_string_literary( turns_until_repaired ), s );
}

int repair_turn_count_for_unit( ShipRepairPort const& port,
                                e_unit_type type ) {
  UNWRAP_CHECK_MSG( turns_to_repair,
                    config_unit_type.composition.unit_types[type]
                        .turns_to_repair,
                    "the unit type '{}' does not specify a "
                    "repair time (is it perhaps not a ship?).",
                    type );
  switch( port.to_enum() ) {
    case ShipRepairPort::e::colony:
      return turns_to_repair.colony;
    case ShipRepairPort::e::european_harbor:
      return turns_to_repair.harbor;
  }
}

void move_damaged_ship_for_repair( SS& ss, TS& ts, Unit& ship,
                                   ShipRepairPort const& port ) {
  int const turns_until_repair =
      repair_turn_count_for_unit( port, ship.type() );
  if( turns_until_repair > 0 )
    // This means that the unit is being marked as damaged and
    // has been damaged for zero turns as of now. Note that this
    // automatically removes the unit from any sentry/fortified
    // status that it had, which is what we want.
    ship.orders() = unit_orders::damaged{
      .turns_until_repair = turns_until_repair };
  else
    // In the OG a Caravel that is sent to the drydock will
    // have a repair turn requirement of 0, and so it is im-
    // mediately activated and can still move in the same
    // turn if it has movement points left. That said, it
    // still needs to be transported to the colony and any
    // units on it need to be destroyed as usual, so even in
    // this case we need to carry out the rest of the actions
    // in this function.
    ship.clear_orders();
  // All units in cargo are destroyed. Note that if the ship is
  // in a colony port then we could theoretically offbooard the
  // units onto land, but instead we will just destroy them 1)
  // for consistency, 2) for simplicity, 3) if we didn't then
  // we'd have to deal with a strange situation where a colony is
  // captured by a foreign power and the ship becomes damaged and
  // moved to a different port (which is what normally happens)
  // and some military units it contains are left on the colony
  // square and reassigned to the other player.
  vector<UnitId> const units_in_cargo = ship.cargo().units();
  for( UnitId const held_id : units_in_cargo )
    UnitOwnershipChanger( ss, held_id ).destroy();
  // In the OG any commodities that remain in the ship after an
  // attacking ship has seized some will be destroyed, although
  // the user is not notified of this.
  ship.cargo().clear_commodities();
  // Now send the ship for repair.
  SWITCH( port ) {
    CASE( colony ) {
      // This can be non-interactive because there is already a
      // colony on the square, and the ship is damaged, so it
      // shouldn't really trigger anything interactive when we
      // move it into the colony.
      UnitOwnershipChanger( ss, ship.id() )
          .change_to_map_non_interactive(
              ts, ss.colonies.colony_for( colony.id ).location );
      break;
    }
    CASE( european_harbor ) {
      unit_move_to_port( ss, ship.id() );
      break;
    }
  }
}

string ship_repair_port_name( SSConst const& ss, e_player player,
                              ShipRepairPort const& port ) {
  SWITCH( port ) {
    CASE( colony ) {
      return ss.colonies.colony_for( colony.id ).name;
    }
    CASE( european_harbor ) {
      return player_obj( player ).harbor_city_name;
    }
  }
}

string ship_damaged_reason( e_ship_damaged_reason reason ) {
  switch( reason ) {
    case e_ship_damaged_reason::battle:
      return "in battle";
    case e_ship_damaged_reason::colony_abandoned:
    case e_ship_damaged_reason::colony_starved:
      return "during colony collapse";
  }
}

string ship_damaged_message( SSConst const& ss,
                             e_player const player_type,
                             e_unit_type ship_type,
                             e_ship_damaged_reason reason,
                             ShipRepairPort const& port ) {
  Player const& player =
      player_for_player_or_die( ss.players, player_type );
  return fmt::format(
      "[{}] [{}] damaged {}! Ship sent to [{}] for repairs.",
      player_possessive( player ), unit_attr( ship_type ).name,
      ship_damaged_reason( reason ),
      ship_repair_port_name( ss, player_type, port ) );
}

string ship_damaged_no_port_message(
    Player const& player, e_unit_type ship_type,
    e_ship_damaged_reason reason ) {
  return fmt::format(
      "[{}] [{}] damaged {}! As there are no available repair "
      "ports, the ship has been lost.",
      player_possessive( player ), unit_attr( ship_type ).name,
      ship_damaged_reason( reason ) );
}

maybe<string> units_lost_on_ship_message( Unit const& ship ) {
  int const num_units_lost =
      ship.cargo().count_items_of_type<Cargo::unit>();
  if( num_units_lost == 0 ) return nothing;
  if( num_units_lost == 1 )
    return fmt::format( "[One] unit onboard has been lost." );
  return fmt::format(
      "[{}] units onboard have been lost.",
      base::capitalize_initials(
          base::int_to_string_literary( num_units_lost ) ) );
}

} // namespace rn
