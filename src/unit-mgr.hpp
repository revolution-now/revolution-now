/****************************************************************
**unit-mgr.hpp
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

// rds
#include "unit-mgr.rds.hpp"

// Revolution Now
#include "error.hpp"
#include "unit-deleted.hpp"
#include "unit.hpp"
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/native-enums.rds.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <functional>
#include <unordered_set>

namespace rn {

struct ColoniesState;
struct Player;
struct SS;
struct TS;
struct UnitsState;

/****************************************************************
** Units
*****************************************************************/
std::string debug_string( UnitsState const& units_state,
                          UnitId            id );

// This gets the activity that the unit is currently engaged in.
// If it is working in a colony then it will be whatever job it
// is doing. If it is e.g. a soldier on the map then it will be
// that. This has nothing to do with the expertise of the unit.
maybe<e_unit_activity> current_activity_for_unit(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, UnitId id );

/****************************************************************
** Map Ownership
*****************************************************************/
// This will give all units that are on a square or are cargo of
// units on that square. This should not recurse more than one
// level deep (beyond the first) because it is a game rule that a
// unit cannot be held as cargo if it itself if capable of
// holding cargo (e.g., a ship can't hold a wagon as cargo).
std::vector<UnitId> euro_units_from_coord_recursive(
    UnitsState const& units_state, Coord coord );

// These will return the coordinates for a unit if it is owned by
// the map or if it is the cargo of something that is owned by
// the map. So this would fail to return a value if e.g. the unit
// is working in a colony or is not yet in the new world.
ND Coord coord_for_unit_indirect_or_die(
    UnitsState const& units_state, GenericUnitId id );

ND maybe<Coord> coord_for_unit_indirect(
    UnitsState const& units_state, GenericUnitId id );

// This will return true for a unit if it is owned by the map or
// if its owner is on the map.
bool is_unit_on_map_indirect( UnitsState const& units_state,
                              UnitId            id );

// These will return true for a unit if it is directly on the
// map.
bool is_unit_on_map( UnitsState const& units_state, UnitId id );

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitsState const& units_state,
                               UnitId            id );

/****************************************************************
** Creation
*****************************************************************/
// Creates a unit that is registered (with a valid ID) but with
// no ownership.
UnitId       create_free_unit( UnitsState&     units_state,
                               Player const&   player,
                               UnitComposition comp );
NativeUnitId create_free_unit( SS& ss, e_native_unit_type type );

// Create unit that is not registered in the unit database, and
// thus has no ID and no ownership. The unit will always have
// id=0, since a unit does not get assigned an ID until it is
// added into the units database.
Unit       create_unregistered_unit( Player const&   player,
                                     UnitComposition comp );
NativeUnit create_unregistered_unit( e_native_unit_type type );

// This has to return a maybe because the unit could theoreti-
// cally by placed on an LCR square, and, since this is the
// coroutine version, the LCR will be explored and one of the
// outcomes is that the unit could be lost.
wait<maybe<UnitId>> create_unit_on_map( SS& ss, TS& ts,
                                        Player&         player,
                                        UnitComposition comp,
                                        Coord           coord );

// Note: when calling from a coroutine, call the coroutine ver-
// sion above since it will run through any UI actions.
UnitId create_unit_on_map_non_interactive( SS& ss, TS& ts,
                                           Player const& player,
                                           UnitComposition comp,
                                           Coord coord );

NativeUnitId create_unit_on_map_non_interactive(
    SS& ss, e_native_unit_type type, Coord coord,
    DwellingId dwelling_id );

/****************************************************************
** Type/Nation Change.
*****************************************************************/
void change_unit_type( SS& ss, TS& ts, Unit& unit,
                       UnitComposition new_comp );

void change_unit_nation( SS& ss, TS& ts, Unit& unit,
                         e_nation new_nation );

/****************************************************************
** Native-specific
*****************************************************************/
// This will check-fail if the unit is free and thus is not asso-
// ciated with a dwelling or tribe.
e_tribe tribe_for_unit( SSConst const&    ss,
                        NativeUnit const& native_unit );

/****************************************************************
** Multi
*****************************************************************/
// These functions apply to multiple types of ownership.

// This will return the coordinate for the unit whenever it is
// possible to map the unit to a coordinate, e.g., applies to map
// ownership, cargo ownership (where holder is on map), colony
// ownership.
maybe<Coord> coord_for_unit_multi_ownership( SSConst const& ss,
                                             GenericUnitId  id );
Coord coord_for_unit_multi_ownership_or_die( SSConst const& ss,
                                             GenericUnitId  id );

/****************************************************************
** Unit Ownership changes.
*****************************************************************/
// All normal game code should use these methods whenever a
// unit's ownership is changed. The interactive version should be
// used where possible.
wait<maybe<UnitDeleted>> unit_ownership_change(
    SS& ss, UnitId id, EuroUnitOwnershipChangeTo const& info );

void unit_ownership_change_non_interactive(
    SS& ss, UnitId id, EuroUnitOwnershipChangeTo const& info );

void destroy_unit( SS& ss, UnitId id );

} // namespace rn
