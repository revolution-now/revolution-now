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
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "igui.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/ref.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/conv.hpp"

using namespace std;

namespace rn {

namespace {

struct ColonyWithDrydock {
  ColonyId colony_id = {};
  // Pythagorean distance from the ship to the colony.
  double distance = {};
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ShipRepairPort_t find_repair_port_for_ship(
    SSConst const& ss, e_nation nation, Coord ship_location ) {
  vector<ColonyId> colonies = ss.colonies.for_nation( nation );
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

wait<> show_damaged_ship_message( TS& ts,
                                  int turns_until_repaired ) {
  CHECK_GT( turns_until_repaired, 0 );
  string_view const s = ( turns_until_repaired > 1 ) ? "s" : "";

  string const msg = fmt::format(
      "This ship is [damaged] and has [{}] turn{} remaining "
      "until it is repaired.",
      base::int_to_string_literary( turns_until_repaired ), s );
  co_await ts.gui.message_box( msg );
}

int repair_turn_count_for_unit( ShipRepairPort_t const& port,
                                e_unit_type             type ) {
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

} // namespace rn
