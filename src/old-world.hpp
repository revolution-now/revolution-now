/****************************************************************
**old-world.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-08.
*
* Description:
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "map-updater.hpp"
#include "unit-id.hpp"

namespace rn {

bool is_unit_on_dock( UnitId id );
bool is_unit_inbound( UnitId id );
bool is_unit_outbound( UnitId id );
bool is_unit_in_port( UnitId id );

// FIXME: needs to be nation-specific.
std::vector<UnitId>
old_world_units_on_dock(); // Sorted by arrival.
std::vector<UnitId>
old_world_units_in_port(); // Sorted by arrival.
std::vector<UnitId> old_world_units_inbound();  // to old world
std::vector<UnitId> old_world_units_outbound(); // to new world

// These will take a ship and make it old (new) world-bound (must
// be a ship). If it is already in this state then this is a
// no-op. If it is already bound for the opposite world then it
// is "turned around" with position retained.
//
// If a ship in the new world is told to sail to the new world
// then an error will be thrown since this is likely a logic er-
// ror. Likewise, if a ship in the old-world port is told to sail
// to the old-world then a similar thing happens.
void unit_sail_to_old_world( UnitId id );
void unit_sail_to_new_world( UnitId id );

// Takes a unit (which is required to be in the cargo of a ship
// that is in port) and moves it to the dock.
void unit_move_to_old_world_dock( UnitId id );

enum e_high_seas_result {
  still_traveling,
  arrived_in_old_world,
  arrived_in_new_world
};

// Takes a unit on the high seas and increases the percentage
// completion of its journey. If the percentage reaches 1.0 as a
// result of this call then the state will automatically be tran-
// sitioned to either the old-world or the new world, as appro-
// priate. If this is called with a unit that is not on the high
// seas then an error will be thrown.
e_high_seas_result advance_unit_on_high_seas(
    UnitId id, IMapUpdater& map_updater );

} // namespace rn
