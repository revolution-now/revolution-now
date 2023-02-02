/****************************************************************
**colony-mgr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Main interface for controlling Colonies.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "colony-mgr.rds.hpp"

// Revolution Now
#include "error.hpp"
#include "expect.hpp"
#include "wait.hpp"

// gs
#include "ss/colony-id.hpp"
#include "ss/colony.hpp"
#include "ss/nation.rds.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <string_view>

namespace rn {

struct Planes;
struct SS;
struct SSConst;
struct TS;

struct ColoniesState;
struct Colony;
struct IMapUpdater;
struct Player;
struct Unit;
struct UnitsState;

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, std::string_view name );

valid_or<e_found_colony_err> unit_can_found_colony(
    SSConst const& ss, UnitId founder );

// This will change the nation of the colony and all units that
// are workers in the colony as well as units that are in the
// same map square as the colony.
void change_colony_nation( Colony&     colony,
                           UnitsState& units_state,
                           e_nation    new_nation );

// Before calling this, it should already have been the case that
// `can_found_colony` was called to validate; so it should work,
// and thus if it doesn't, it will check-fail.
ColonyId found_colony( SS& ss, TS& ts, Player const& player,
                       UnitId founder, std::string_view name );

// Evolve the colony by one turn.
wait<> evolve_colonies_for_player( Planes& planes, SS& ss,
                                   TS& ts, Player& player );

// This basically creates a default-constructed colony and gives
// it a nation, name, and location, but nothing more. So it is
// not a valid colony yet. Normal game code shouldn't really call
// this, it is mostly exposed for testing.
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_nation nation, Coord where,
                              std::string_view name );

// Will strip the unit of any commodities (including inventory
// and modifiers) and deposit the commodities into the colony.
void strip_unit_to_base_type( Player const& player, Unit& unit,
                              Colony& colony );

void move_unit_to_colony( UnitsState& units_state,
                          Colony& colony, UnitId unit_id,
                          ColonyJob_t const& job );

// This will put the unit on the map at the location of the
// colony.
void remove_unit_from_colony( UnitsState& units_state,
                              Colony& colony, UnitId unit_id );

void change_unit_outdoor_job( Colony& colony, UnitId id,
                              e_outdoor_job new_job );

int colony_population( Colony const& colony );

// All indoor+outdoor units. Does not include units at the colony
// gate.
std::vector<UnitId> colony_workers( Colony const& colony );

bool colony_has_unit( Colony const& colony, UnitId id );

// This is the method that normal game code should use to destroy
// a colony. References to the colony will be invalid after this.
// Any units at the gate (or ships in port) will be left un-
// touched, but all units working in the colony will be de-
// stroyed. Therefore note that this can leave a ship (previously
// in port) on land. That should be ok, since the game (just like
// the original game) should allow the ship to move off land and
// into the water. If it doesn't do so in time it could theoreti-
// cally be attacked by a foreign unit. When that happens, the
// original game panics, but this game will handle it in a spe-
// cial way (see the orders-move module).
void destroy_colony( SS& ss, IMapUpdater& map_updater,
                     Colony& colony );

// This will do the colony destruction depixelation animation and
// then will actually destroy the colony using the destroy_colony
// method documented above. An optional message will be shown to
// the user first, before any others.
wait<> run_colony_destruction( Planes& planes, SS& ss, TS& ts,
                               Colony&            colony,
                               maybe<std::string> msg );

// Given a colony, find the squares in its surroundings that are
// being worked by units in other colonies, either friendly or
// foreign. In the original game, these squares will be drawn
// with red boxes around them in the colony view to signal that
// no further colonists can work there.
refl::enum_map<e_direction, bool>
find_occupied_surrounding_colony_squares( SSConst const& ss,
                                          Colony const& colony );

// If the colony doesn't already have a stockade and the player
// has Sieur de La Salle and its population >= 3 then it will be
// given a stockade.
void give_stockade_if_needed( Player const& player,
                              Colony&       colony );

} // namespace rn
