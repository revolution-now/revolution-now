/****************************************************************
**harbor-units.hpp
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
#include "harbor-units.rds.hpp"
#include "nation.hpp"
#include "unit-composer.hpp"
#include "unit-id.hpp"

// gs
#include "ss/unit-type.rds.hpp"

namespace rn {

struct ColoniesState;
struct Player;
struct TerrainState;
struct UnitHarborViewState;
struct UnitsState;

bool is_unit_inbound( UnitsState const& units_state, UnitId id );
bool is_unit_outbound( UnitsState const& units_state,
                       UnitId            id );
bool is_unit_in_port( UnitsState const& units_state, UnitId id );

std::vector<UnitId> harbor_units_on_dock(
    UnitsState const& units_state,
    e_nation          nation ); // Sorted by arrival.
std::vector<UnitId> harbor_units_in_port(
    UnitsState const& units_state,
    e_nation          nation ); // Sorted by arrival.
std::vector<UnitId> harbor_units_inbound(
    UnitsState const& units_state,
    e_nation          nation ); // to harbor
std::vector<UnitId> harbor_units_outbound(
    UnitsState const& units_state,
    e_nation          nation ); // to new world

// This is called on a ship that is in the harbor to make it sail
// to the new world. This just puts it into inbound state with
// the appropriate percentage given its current state; it will
// not ever move the unit to the map. It only makes sense to sail
// to the new world if the ship is already in port on on the high
// seas, and so there will be an existing harbor state associated
// with it that is needed in order to properly transition it to
// outbound. In general that state may need to be explicitly
// passed in because the unit may not have that state currently,
// e.g. if it is being drag and dropped, it will be disowned in
// the process and will lose its harbor state. If it is not
// passed in then it will be assumed that the unit already has
// that state.
void unit_sail_to_new_world(
    TerrainState const& terrain_state, UnitsState& units_state,
    Player const& player, UnitId id,
    UnitHarborViewState const& previous_harbor_state );

// This is the one where the unit is assumed to already have a
// harbor state.
void unit_sail_to_new_world( TerrainState const& terrain_state,
                             UnitsState&         units_state,
                             Player const& player, UnitId id );

// This is the method that should always be called to set a ship
// sailing the high seas, or at least moving toward the harbor if
// it is already either on the high seas (in either direction) or
// in port.
//
// If it is a unit owned by the map then it record its position
// so that it can return to that position when it emerges back in
// the new world eventually.
//
// For the same reason that is documented above for the "sail to
// new world" function, we have two variants of these, one which
// takes a previous harbor state and one which does not.
void unit_sail_to_harbor(
    TerrainState const& terrain_state, UnitsState& units_state,
    Player& player, UnitId id,
    UnitHarborViewState const& previous_harbor_state );

// This is the one where the unit's existing state will be
// checked taken from the unit itself (it may or may not be a
// harbor state; e.g. it could be on the map).
void unit_sail_to_harbor( TerrainState const& terrain_state,
                          UnitsState&         units_state,
                          Player& player, UnitId id );

// Takes a unit and just moves it to port (that means the dock if
// it is a non-ship) by just overwriting whatever state it cur-
// rently has (though note that it will preserve the
// `sailed_from`). This can be used in the following circum-
// stances: 1. a unit that is already inbound which as arrived
// and needs to be moved to port, 2. a ship that is damaged and
// needs to be immediately relocated to port, 3. a unit that is
// purchased in europe. Also useful for creating testing setups.
void unit_move_to_port( UnitsState& units_state, UnitId id );

// Takes a unit on the high seas and increases the turn count of
// its journey. If the turn count reaches the maximum as a result
// of this call then the state will automatically be transitioned
// to either the harbor or the new world, as appropriate. If this
// is called with a unit that is not on the high seas then an
// error will be thrown.
[[nodiscard]] e_high_seas_result advance_unit_on_high_seas(
    TerrainState const& terrain_state, UnitsState& units_state,
    Player const& player, UnitId id );

// When a unit arrives in the new world from the high seas we
// need to find a square on which to place the unit. That is ac-
// tually a non-trivial process, and this function does that.
maybe<Coord> find_new_world_arrival_square(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state,
    TerrainState const& terrain_state, Player const& player,
    UnitHarborViewState const& info );

UnitId create_unit_in_harbor( UnitsState& units_state,
                              e_nation    nation,
                              e_unit_type type );

UnitId create_unit_in_harbor( UnitsState&     units_state,
                              e_nation        nation,
                              UnitComposition comp );
} // namespace rn
