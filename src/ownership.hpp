/****************************************************************
* ownership.hpp
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

#include "unit.hpp"

#include <functional>
#include <optional>
#include <unordered_set>

namespace rn {

Unit& unit_from_id( UnitId id );
UnitIdVec units_all( std::optional<e_nation> n = std::nullopt );
// Apply a function to all units. The function may mutate the
// units.
void map_units( std::function<void( Unit& )> func );

// Not safe, probably temporary.
UnitId create_unit_on_map( e_unit_type type, Y y, X x );
// Functions for mapping between units and coordinates on the
// map.
std::unordered_set<UnitId> const& units_from_coord( Y y, X x );
UnitIdVec units_int_rect( Rect const& rect );
Coord coords_for_unit( UnitId id );
OptCoord coords_for_unit_safe( UnitId id );

// Do not call directly. Changes a unit's ownership from whatever
// it is (map or otherwise) to the map at the given coordinate.
// It will always move the unit to the target square without
// question (checking only that the unit exists). NOTE: this is a
// low-level function; it does not do any checking, and should
// not be called directly. E.g., this function will happily move
// a land unit into water.
void ownership_change_to_map( UnitId id, Coord target );

void ownership_change_to_cargo( UnitId new_holder, UnitId held );

} // namespace rn
