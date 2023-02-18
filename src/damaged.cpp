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

// ss
#include "ss/colonies.hpp"
#include "ss/ref.hpp"

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

} // namespace rn
