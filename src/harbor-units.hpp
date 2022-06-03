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
#include "map-updater.hpp"
#include "nation.hpp"
#include "unit-composer.hpp"
#include "unit-id.hpp"

// Rds
#include "utype.rds.hpp"

namespace rn {

struct UnitsState;
struct Player;

bool is_unit_inbound( UnitsState const& units_state, UnitId id );
bool is_unit_outbound( UnitsState const& units_state,
                       UnitId            id );
bool is_unit_in_port( UnitsState const& units_state, UnitId id );

// FIXME: needs to be nation-specific.
std::vector<UnitId> harbor_units_on_dock(
    UnitsState const& units_state ); // Sorted by arrival.
std::vector<UnitId> harbor_units_in_port(
    UnitsState const& units_state ); // Sorted by arrival.
std::vector<UnitId> harbor_units_inbound(
    UnitsState const& units_state ); // to harbor
std::vector<UnitId> harbor_units_outbound(
    UnitsState const& units_state ); // to new world

// This is called on a ship that is in the harbor to make it sail
// to the new world.
void unit_sail_to_new_world( UnitsState& units_state,
                             UnitId      id );

// This is the method that should always be called to set a ship
// sailing the high seas, or at least moving toward the harbor if
// it is already either on the high seas (in either direction) or
// in port. If it is a unit owned by the map then it record its
// position so that it can return to that position when it
// emerges back in the new world eventually.
void unit_sail_to_harbor( UnitsState& units_state,
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

enum e_high_seas_result {
  still_traveling,
  arrived_in_harbor,
  arrived_in_new_world
};

// Takes a unit on the high seas and increases the percentage
// completion of its journey. If the percentage reaches 1.0 as a
// result of this call then the state will automatically be tran-
// sitioned to either the harbor or the new world, as appropri-
// ate. If this is called with a unit that is not on the high
// seas then an error will be thrown.
e_high_seas_result advance_unit_on_high_seas(
    UnitsState& units_state, Player const& player, UnitId id,
    IMapUpdater& map_updater );

UnitId create_unit_in_harbor( UnitsState& units_state,
                              e_nation    nation,
                              e_unit_type type );

UnitId create_unit_in_harbor( UnitsState&     units_state,
                              e_nation        nation,
                              UnitComposition comp );
} // namespace rn
