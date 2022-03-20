/****************************************************************
**ustate.hpp
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
#include "colony.hpp"
#include "error.hpp"
#include "unit.hpp"

// base
#include "base/function-ref.hpp"

// Rds
#include "ustate.rds.hpp"

// C++ standard library
#include <functional>
#include <unordered_set>

namespace rn {

/****************************************************************
** Units
*****************************************************************/
std::string debug_string( UnitId id );

ND bool unit_exists( UnitId id );
// FIXME: replace this with UnitsState::unit_for.
ND Unit&            unit_from_id( UnitId id );
std::vector<UnitId> units_all();
std::vector<UnitId> units_all( e_nation n );
// Apply a function to all units. The function may mutate the
// units. NOTE: here, the word "map" is meant in the functional
// programming sense, and not in the sense of the game world map.
void map_units( base::function_ref<void( Unit& )> func );
void map_units( e_nation                          nation,
                base::function_ref<void( Unit& )> func );

// Should not be holding any references to the unit after this.
void destroy_unit( UnitId id );

/****************************************************************
** Map Ownership
*****************************************************************/
// Function for mapping between units and coordinates on the map.
// These will only give the units that are owned immediately by
// the map; it will not give units who are cargo of those units.
// TODO: replace usage of this with UnitsState::from_coord.
ND std::unordered_set<UnitId> const& units_from_coord(
    Coord const& c );

// This will give all units that are on a square or are cargo of
// units on that square. This should not recurse more than one
// level deep (beyond the first) because it is a game rule that a
// unit cannot be held as cargo if it itself if capable of
// holding cargo (e.g., a ship can't hold a wagon as cargo).
std::vector<UnitId> units_from_coord_recursive( Coord coord );

// This is in the rare cases that we need to change a unit's po-
// sition manually, such as when e.g. a colonist is captured in
// combat.  Unit must be owned by the map for this!
void move_unit_from_map_to_map( UnitId id, Coord dest );

std::vector<UnitId> units_in_rect( Rect const& rect );

// Get all units in the eight squares that surround coord.
std::vector<UnitId> surrounding_units( Coord const& coord );

// Returns the map coordinates for the unit if it is on the map
// (which does NOT include being cargo of a unit on the map; for
// that, see `coord_for_unit_indirect`).
// TODO: replace usages of this with UnitsState::maybe_coord_for,
// or UnitsState::coord_for.
maybe<Coord> coord_for_unit( UnitId id );

// These will return the coordinates for a unit if it is owned by
// the map or the coordinates of its owner if it is ultimately
// owned by something that is on the map. This would fail to re-
// turn a value if e.g. the unit is not yet in the new world.
ND Coord coord_for_unit_indirect_or_die( UnitId id );
ND maybe<Coord> coord_for_unit_indirect( UnitId id );

// This will return true for a unit if it is owned by the map or
// if its owner is on the map.
bool is_unit_on_map_indirect( UnitId id );

// These will return true for a unit if it is directly on the
// map.
bool is_unit_on_map( UnitId id );

/****************************************************************
** Colony Ownership
*****************************************************************/
// This returns all units that are either working in the colony
// or who are on the map on the colony square.
std::unordered_set<UnitId> units_at_or_in_colony( ColonyId id );

// If the unit is working in the colony then this will return it;
// however it will not return a ColonyId if the unit simply occu-
// pies the same square as the colony.
maybe<ColonyId> colony_for_unit_who_is_worker( UnitId id );

bool is_unit_in_colony( UnitId id );

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitId id );

/****************************************************************
** Old World View Ownership
*****************************************************************/
base::valid_or<generic_err> check_old_world_state_invariants(
    UnitOldWorldViewState_t const& info );

// If unit is owned by old-world-view then this will return info.
maybe<UnitOldWorldViewState_t&> unit_old_world_view_info(
    UnitId id );

// Get a set of all units owned by the old-world-view.
// FIXME: needs to be nation-specific.
std::vector<UnitId> units_in_old_world_view();

/****************************************************************
** Creation
*****************************************************************/
// Creates a unit with no ownership.
UnitId create_unit( e_nation nation, UnitComposition comp );
UnitId create_unit( e_nation nation, UnitType type );

/****************************************************************
** Multi
*****************************************************************/
// These functions apply to multiple types of ownership.

// This will return the coordinate for the unit whenever it is
// possible to map the unit to a coordinate, e.g., applies to map
// ownership, cargo ownership (where holder is on map), colony
// ownership.
maybe<Coord> coord_for_unit_multi_ownership( UnitId id );
Coord        coord_for_unit_multi_ownership_or_die( UnitId id );

/****************************************************************
** For Testing / Development Only
*****************************************************************/
// Do not call these in normal game code.
UnitId create_unit_on_map( e_nation nation, UnitComposition comp,
                           Coord coord );

} // namespace rn
