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

// Revolution Now
#include "error.hpp"
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/unit-id.hpp"
#include "ss/unit.hpp"

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
struct SSConst;
struct TS;
struct Tribe;
struct UnitsState;
struct UnitComposition;

enum class e_native_unit_type;
enum class e_tribe;

/****************************************************************
** Units
*****************************************************************/
std::string debug_string( UnitsState const& units_state,
                          UnitId id );

// This gets the activity that the unit is currently engaged in.
// If it is working in a colony then it will be whatever job it
// is doing. If it is e.g. a soldier on the map then it will be
// that. This has nothing to do with the expertise of the unit.
maybe<e_unit_activity> current_activity_for_unit(
    UnitsState const& units_state,
    ColoniesState const& colonies_state, UnitId id );

/****************************************************************
** Map Ownership
*****************************************************************/
// Only units that are directly on the map.
std::vector<UnitId> euro_units_from_coord(
    UnitsState const& units_state, gfx::point tile );

// This will give all units that are on a square or are cargo of
// units on that square. This should not recurse more than one
// level deep (beyond the first) because it is a game rule that a
// unit cannot be held as cargo if it itself if capable of
// holding cargo (e.g., a ship can't hold a wagon as cargo).
std::vector<UnitId> euro_units_from_coord_recursive(
    UnitsState const& units_state, gfx::point tile );

// Same as above but will limit to a single player. Thus, we will
// either return all units of a single player, or nothing, at
// least under normal gamie operation.
std::vector<UnitId> euro_units_from_coord_recursive(
    UnitsState const& units_state, e_player player,
    gfx::point tile );

// Same as above but also includes any native units.
std::vector<GenericUnitId> units_from_coord_recursive(
    UnitsState const& units, gfx::point tile );

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
                              UnitId id );

// These will return true for a unit if it is directly on the
// map.
bool is_unit_on_map( UnitsState const& units_state, UnitId id );

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitsState const& units_state,
                               UnitId id );

// This is called on a colony square just before it is attacked.
// It will eject all units (not commodities) from the cargo of
// any ships that are on the square and mark those offboarded
// units as sentried. This is to remove the ambiguity of what
// happens when there are units on a ship in a colony port and
// that colony is either attacked and/or destroyed. By first off-
// boarding all units we are guaranteed to then replicate the be-
// havior of the OG which does not have the concept of units on
// ships technically. Returns a list of the units that were off-
// boarded.
std::vector<UnitId> offboard_units_on_ships( SS& ss, TS& ts,
                                             Coord coord );

// Offboards any units and puts them on the map, which should be
// land in order to uphold game rules, though this function won't
// check for that.
std::vector<UnitId> offboard_units_on_ship( SS& ss, TS& ts,
                                            Unit& ship );

/****************************************************************
** Creation
*****************************************************************/
// Creates a unit that is registered (with a valid ID) but with
// no ownership.
UnitId create_free_unit( UnitsState& units_state,
                         Player const& player,
                         UnitComposition const& comp );

// Create unit that is not registered in the unit database, and
// thus has no ID and no ownership. The unit will always have
// id=0, since a unit does not get assigned an ID until it is
// added into the units database.
Unit create_unregistered_unit( Player const& player,
                               UnitComposition const& comp );
NativeUnit create_unregistered_unit( e_native_unit_type type );

// This has to return a maybe because the unit could theoreti-
// cally by placed on an LCR square, and, since this is the
// coroutine version, the LCR will be explored and one of the
// outcomes is that the unit could be lost.
wait<maybe<UnitId>> create_unit_on_map(
    SS& ss, TS& ts, Player& player, UnitComposition const& comp,
    Coord coord );

// Note: when calling from a coroutine, call the coroutine ver-
// sion above since it will run through any UI actions.
UnitId create_unit_on_map_non_interactive(
    SS& ss, TS& ts, Player const& player,
    UnitComposition const& comp, gfx::point coord );

NativeUnitId create_unit_on_map_non_interactive(
    SS& ss, e_native_unit_type type, gfx::point coord,
    DwellingId dwelling_id );

/****************************************************************
** Destruction.
*****************************************************************/
// This will safely destroy multiple units, accounting for e.g.
// the fact that one unit may be in the cargo of another unit in
// the list.
void destroy_units( SS& ss, std::vector<UnitId> const& units );

/****************************************************************
** Type/Player Change.
*****************************************************************/
void change_unit_type( SS& ss, TS& ts, Unit& unit,
                       UnitComposition const& new_comp );

void change_unit_player( SS& ss, TS& ts, Unit& unit,
                         e_player new_player );

// This is for when we want to change the unit's player and move
// them in one go. This is useful for a unit capture in order to
// avoid changing the unit's player first and inadvertantly re-
// vealing map square around it before moving it.
void change_unit_player_and_move( SS& ss, TS& ts, Unit& unit,
                                  e_player new_player,
                                  Coord target );

/****************************************************************
** Native-specific
*****************************************************************/
Tribe const& tribe_for_unit( SSConst const& ss,
                             NativeUnit const& native_unit );
Tribe& tribe_for_unit( SS& ss, NativeUnit const& native_unit );

e_tribe tribe_type_for_unit( SSConst const& ss,
                             NativeUnit const& native_unit );

// When sorting order doesn't matter use this as it should be
// faster.
std::vector<NativeUnitId> units_for_tribe_unordered(
    SSConst const& ss, e_tribe target_tribe_type );

// Uses a set so that they will automatically be sorted.
std::set<NativeUnitId> units_for_tribe_ordered(
    SSConst const& ss, e_tribe target_tribe_type );

/****************************************************************
** Multi
*****************************************************************/
// These functions apply to multiple types of ownership.

// This will return the coordinate for the unit whenever it is
// possible to map the unit to a coordinate, e.g., applies to map
// ownership, cargo ownership (where holder is on map), colony
// ownership.
maybe<Coord> coord_for_unit_multi_ownership( SSConst const& ss,
                                             GenericUnitId id );
Coord coord_for_unit_multi_ownership_or_die( SSConst const& ss,
                                             GenericUnitId id );

} // namespace rn
