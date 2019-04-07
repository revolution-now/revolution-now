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

// Revolution Now
#include "aliases.hpp"
#include "unit.hpp"

// C++ standard library
#include <functional>
#include <optional>
#include <unordered_set>

namespace rn {

ND bool unit_exists( UnitId id );
ND Unit&    unit_from_id( UnitId id );
Vec<UnitId> units_all(
    std::optional<e_nation> n = std::nullopt );
// Apply a function to all units. The function may mutate the
// units.
void map_units( std::function<void( Unit& )> const& func );

// Should not be holding any references to the unit after this.
void destroy_unit( UnitId id );

// Not safe, probably temporary.
ND UnitId create_unit_on_map( e_nation nation, e_unit_type type,
                              Y y, X x );
// Functions for mapping between units and coordinates on the
// map. These will only give the units that are owned immediately
// by the map; it will not give units who are cargo of those
// units.
ND std::unordered_set<UnitId> const& units_from_coord( Y y,
                                                       X x );
ND std::unordered_set<UnitId> const& units_from_coord( Coord c );

// This will give all units that are on a square or are cargo of
// units on that square. This should not recurse more than one
// level deep (beyond the first) because it is a game rule that a
// unit cannot be held as cargo if it itself if capable of
// holding cargo (e.g., a ship can't hold a wagon as cargo).
Vec<UnitId> units_from_coord_recursive( Coord coord );

ND Opt<e_nation> nation_from_coord( Coord coord );

// This is in the rare cases that we need to change a unit's po-
// sition manually, such as when e.g. a colonist is captured in
// combat.  Unit must be owned by the map for this!
void move_unit_from_map_to_map( UnitId id, Coord dest );

ND Vec<UnitId> units_in_rect( Rect const& rect );

// These will return the coordinates for a unit if it is owned by
// the map or the coordinates of its owner if it is ultimately
// owned by something that is on the map. This would fail to re-
// turn a value if e.g. the unit is not yet in the new world.
ND Coord coords_for_unit( UnitId id );
ND OptCoord coords_for_unit_safe( UnitId id );

// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nullopt otherwise.
Opt<UnitId> is_unit_onboard( UnitId id );

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
