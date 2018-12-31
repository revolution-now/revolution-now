/****************************************************************
**ownership.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles creation, destruction, and ownership of
*              units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "unit.hpp"

#include <functional>
#include <optional>
#include <unordered_set>

namespace rn {

ND Unit&  unit_from_id( UnitId id );
UnitIdVec units_all( std::optional<e_nation> n = std::nullopt );
// Apply a function to all units. The function may mutate the
// units.
void map_units( std::function<void( Unit& )> const& func );

// Not safe, probably temporary.
ND UnitId create_unit_on_map( e_nation nation, e_unit_type type,
                              Y y, X x );
// Functions for mapping between units and coordinates on the
// map.
ND std::unordered_set<UnitId> const& units_from_coord( Y y,
                                                       X x );
ND std::unordered_set<UnitId> const& units_from_coord( Coord c );

ND UnitIdVec units_in_rect( Rect const& rect );

// These will return the coordinates for a unit if it is owned by
// the map or the coordinates of its owner if it is ultimately
// owned by something that is on the map. This would fail to re-
// turn a value if e.g. the unit is not yet in the new world.
ND Coord coords_for_unit( UnitId id );
ND OptCoord coords_for_unit_safe( UnitId id );

// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nullopt otherwise.
OptUnitId is_unit_onboard( UnitId id );

std::string debug_string( UnitId id );

/****************************************************************
** Do not call directly
*****************************************************************/
// Changes a unit's ownership from whatever it is (map or other-
// wise) to the map at the given coordinate. It will always move
// the unit to the target square without question (checking only
// that the unit exists). NOTE: this is a low-level function; it
// does not do any checking, and should not be called directly.
// E.g., this function will happily move a land unit into water.
void ownership_change_to_map( UnitId id, Coord const& target );

void ownership_change_to_cargo( UnitId new_holder, UnitId held );

// Removes unit from any ownership. Used when transitioning own-
// ership.
void ownership_disown_unit( UnitId id );

} // namespace rn
