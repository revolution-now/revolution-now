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
#include "adt.hpp"
#include "aliases.hpp"
#include "unit.hpp"

// function_ref
#include "function_ref.hpp"

// C++ standard library
#include <functional>
#include <optional>
#include <unordered_set>

namespace rn {

std::string debug_string( UnitId id );

ND bool unit_exists( UnitId id );
ND Unit&    unit_from_id( UnitId id );
Vec<UnitId> units_all(
    std::optional<e_nation> n = std::nullopt );
// Apply a function to all units. The function may mutate the
// units. NOTE: here, the word "map" is meant in the functional
// programming sense, and not in the sense of the game world map.
void map_units( tl::function_ref<void( Unit& )> func );

// Should not be holding any references to the unit after this.
void destroy_unit( UnitId id );

/****************************************************************
** Map Ownership
*****************************************************************/
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
ND Opt<Coord> coords_for_unit_safe( UnitId id );

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nullopt otherwise.
Opt<UnitId> is_unit_onboard( UnitId id );

/****************************************************************
** EuroPort View Ownership
*****************************************************************/
// These pertain to units who are owned by either the high seas
// or by europe view (e.g., in port, on the dock, etc.);
ADT_RN(
    UnitEuroPortViewState,
    // For ships that are venturing to europe. `percent` starts
    // from 0 and goes to 1.0 at arrival.
    ( outbound,              //
      ( double, percent ) ), //
    // For ships that are traveling from europe to the new world.
    // `percent` starts from 0 and goes to 1.0 at arrival.
    ( inbound,               //
      ( double, percent ) ), //
    // If a ship is in this state then it is in port (shown in
    // the "in port" box) whereas for land units this means that
    // they are on the dock.
    ( in_port ) //
);

// If unit is owned by euro-port-view then this will return info.
Opt<Ref<UnitEuroPortViewState_t>> unit_euro_port_view_info(
    UnitId id );

// Get a set of all units owned by the euro-port-view.
Vec<UnitId> units_in_euro_port_view();

/****************************************************************
** For Testing / Development Only
*****************************************************************/
// Do not call these in normal game code.
UnitId create_unit_on_map( e_nation nation, e_unit_type type,
                           Y y, X x );
UnitId create_unit_in_euroview_port( e_nation    nation,
                                     e_unit_type type );

UnitId create_unit_as_cargo( e_nation nation, e_unit_type type,
                             UnitId holder );

// Creates a unit with no ownership.
Unit& create_unit( e_nation nation, e_unit_type type );

/****************************************************************
** Changing Unit Ownership
*****************************************************************/
// Changes a unit's ownership from whatever it is (map or other-
// wise) to the map at the given coordinate. It will always move
// the unit to the target square without question (checking only
// that the unit exists). NOTE: this is a low-level function; it
// does not do any checking, and should not be called directly.
// E.g., this function will happily move a land unit into water.
void ownership_change_to_map( UnitId id, Coord const& target );

void ownership_change_to_cargo( UnitId new_holder, UnitId held );
void ownership_change_to_cargo( UnitId new_holder, UnitId held,
                                int slot );

void ownership_change_to_euro_port_view(
    UnitId id, UnitEuroPortViewState_t info );

/****************************************************************
** Do not call directly
*****************************************************************/
// Removes unit from any ownership. Used when transitioning own-
// ership. This is only in the header file because it needs to be
// listed as the `friend` of some classes.
namespace internal {
void ownership_disown_unit( UnitId id );
}

} // namespace rn
