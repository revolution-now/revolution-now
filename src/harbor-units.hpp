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
#include "unit-deleted.hpp"
#include "unit-id.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/unit-composition.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Player;
struct SS;
struct SSConst;
struct TS;
struct UnitsState;

/****************************************************************
** Querying.
*****************************************************************/
bool is_unit_inbound( UnitsState const& units_state, UnitId id );
bool is_unit_outbound( UnitsState const& units_state,
                       UnitId id );
bool is_unit_in_port( UnitsState const& units_state, UnitId id );

// NOTE: these will return the list of units ordered as they are
// in the ordering map.
std::vector<UnitId> harbor_units_on_dock(
    UnitsState const& units_state,
    e_player player ); // Sorted by arrival.
std::vector<UnitId> harbor_units_in_port(
    UnitsState const& units_state,
    e_player player ); // Sorted by arrival.
std::vector<UnitId> harbor_units_inbound(
    UnitsState const& units_state,
    e_player player ); // to harbor
std::vector<UnitId> harbor_units_outbound(
    UnitsState const& units_state,
    e_player player ); // to new world

/****************************************************************
** Harbor ownership changes.
*****************************************************************/
// This is called on a ship that is in the harbor to make it sail
// to the new world. This just puts it into inbound state with
// the appropriate percentage given its current state; it will
// not ever move the unit to the map.
void unit_sail_to_new_world( SS& ss, UnitId id );

// This is the method that should always be called to set a ship
// sailing the high seas, or at least moving toward the harbor if
// it is already either on the high seas (in either direction) or
// in port. If it is a unit owned by the map then it record its
// position so that it can return to that position when it
// emerges back in the new world eventually.
void unit_sail_to_harbor( SS& ss, UnitId id );

// Takes a unit and just moves it to port (that means the dock if
// it is a non-ship) by just overwriting whatever state it cur-
// rently has (though note that it will preserve the
// `sailed_from`). This can be used in the following circum-
// stances: 1. a unit that is already inbound which as arrived
// and needs to be moved to port, 2. a ship that is damaged and
// needs to be immediately relocated to port, 3. a unit that is
// purchased in europe. Also useful for creating testing setups.
void unit_move_to_port( SS& ss, UnitId id );

// Takes a unit on the high seas and increases the turn count of
// its journey. If the turn count reaches the maximum as a result
// of this call then the state will automatically be transitioned
// to either the harbor or the new world, as appropriate. If this
// is called with a unit that is not on the high seas then an
// error will be thrown.
[[nodiscard]] e_high_seas_result advance_unit_on_high_seas(
    SS& ss, Player& player, UnitId id );

/****************************************************************
** Miscellaneous.
*****************************************************************/
// When a ship appears in the new world (e.g. it arrives from the
// high seas, or it was created) we need to find a square on
// which to place the unit. That is actually a non-trivial
// process, and this function does that. If the unit was previ-
// ously in the new world and sailed to the harbor then you can
// pass the square that it sailed from as a preference for where
// the ship should be placed.
maybe<Coord> find_new_world_arrival_square(
    SSConst const& ss, TS& ts, Player const& player,
    maybe<Coord> sailed_from );

// This will check if there are ships in port and, if so, it will
// ensure that at least one of them are selected (for a good
// player experience, so that they never go to the harbor view
// and see ships sitting there and none of them are selected).
void update_harbor_selected_unit( UnitsState const& units_state,
                                  Player& player );

// This is to be called just after a ship that was in port is
// sent to sail to the new world, and will check if there is an-
// other ship in port and, if so, will select that unit for a
// better UX. The difference between this one and the one above
// (update_harbor_selected_unit) is that this one will always
// prefer an in-port unit if there is one, whereas the one above
// is happy with any unit in the harbor being selected.
void try_select_in_port_ship( UnitsState const& units,
                              Player& player );

UnitId create_unit_in_harbor( SS& ss, Player& player,
                              UnitComposition comp );

} // namespace rn
