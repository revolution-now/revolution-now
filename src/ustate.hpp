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
#include "error.hpp"
#include "unit-id.hpp"
#include "unit.hpp"
#include "wait.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <functional>
#include <unordered_set>

namespace rn {

struct ColoniesState;
struct IGui;
struct IMapUpdater;
struct Player;
struct SettingsState;
struct TerrainState;
struct UnitsState;

/****************************************************************
** Units
*****************************************************************/
std::string debug_string( UnitsState const& units_state,
                          UnitId            id );

// FIXME: replace this with UnitsState::unit_for.
ND Unit& unit_from_id( UnitId id );

// This gets the activity that the unit is currently engaged in.
// If it is working in a colony then it will be whatever job it
// is doing. If it is e.g. a soldier on the map then it will be
// that. This has nothing to do with the expertise of the unit.
maybe<e_unit_activity> current_activity_for_unit(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, UnitId id );

/****************************************************************
** Unit Promotion
*****************************************************************/
// This is the function that top-level game logic should call
// whenever it is determined that a unit is up for promotion (the
// it is ok if the unit cannot be promoted further). This will
// find the unit's current activity and promote it along those
// lines if a promotion target exists. It will return true if a
// promotion was possible (and was thus made). If the unit has no
// activity the no promotion will be made. Technically, in that
// case, we could still theoretically promote e.g. an indentured
// servant to a free colonist, but the game will never do that
// (outside of cheat mode) when the unit has no activity, so we
// don't do that. Also, note that if the unit is an expert at
// something other than the given activity then no promotion will
// happen.
//
// TODO: promote veterans to continentals after independence.
//
bool try_promote_unit_for_current_activity(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, Unit& unit );

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

ND maybe<Coord> coord_for_unit_indirect(
    UnitsState const& units_state, UnitId id );
// FIXME: deprecated
ND maybe<Coord> coord_for_unit_indirect( UnitId id );

// This will return true for a unit if it is owned by the map or
// if its owner is on the map.
bool is_unit_on_map_indirect( UnitId id );

// These will return true for a unit if it is directly on the
// map.
bool is_unit_on_map( UnitId id );

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitId id );

/****************************************************************
** Creation
*****************************************************************/
// Creates a unit with no ownership.
UnitId create_unit( UnitsState& units_state, e_nation nation,
                    UnitComposition comp );
UnitId create_unit( UnitsState& units_state, e_nation nation,
                    UnitType type );
UnitId create_unit( UnitsState& units_state, e_nation nation,
                    e_unit_type type );

wait<UnitId> create_unit_on_map(
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, SettingsState const& settings, IGui& gui,
    IMapUpdater& map_updater, UnitComposition comp,
    Coord coord );

// Note: when calling from a coroutine, call the coroutine ver-
// sion above since it will run through any UI actions.
UnitId create_unit_on_map_non_interactive(
    UnitsState& units_state, IMapUpdater& map_updater,
    e_nation nation, UnitComposition comp, Coord coord );

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

// Create unit with no ownership. Note that the unit will always
// have id=0, since a unit does not get assigned an ID until it
// is added into a UnitsState with some ownership.
Unit create_free_unit( e_nation nation, UnitComposition comp );

} // namespace rn
