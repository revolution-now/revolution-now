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

// Revolution Now
#include "colony-id.hpp"
#include "colony.hpp"
#include "coord.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "map-updater.hpp"
#include "nation.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// Rds
#include "colony-mgr.rds.hpp"

// C++ standard library
#include <string_view>

namespace rn {

struct IGui;
struct SettingsState;
struct ColoniesState;
struct Player;
struct TerrainState;
struct UnitsState;
struct Colony;
struct Unit;

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, std::string_view name );

valid_or<e_found_colony_err> unit_can_found_colony(
    ColoniesState const& colonies_state,
    UnitsState const&    units_state,
    TerrainState const& terrain_state, UnitId founder );

// This will change the nation of the colony and all units that
// are workers in the colony as well as units that are in the
// same map square as the colony.
void change_colony_nation( Colony&     colony,
                           UnitsState& units_state,
                           e_nation    new_nation );

// Before calling this, it should already have been the case that
// `can_found_colony` was called to validate; so it should work,
// and thus if it doesn't, it will check-fail.
ColonyId found_colony_unsafe( ColoniesState&      colonies_state,
                              TerrainState const& terrain_state,
                              UnitsState&         units_state,
                              UnitId              founder,
                              IMapUpdater&        map_updater,
                              std::string_view    name );

// Evolve the colony by one turn.
wait<> evolve_colonies_for_player(
    ColoniesState& colonies_state, SettingsState const& settings,
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, IMapUpdater& map_updater, IGui& gui );

// This basically creates a default-constructed colony and gives
// it a nation, name, and location, but nothing more. So it is
// not a valid colony yet. Normal game code shouldn't really call
// this, it is mostly exposed for testing.
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_nation nation, Coord where,
                              std::string_view name );

// Will strip the unit of any commodities (including inventory
// and modifiers) and deposit them commodities into the colony.
// The unit must be on the colony square otherwise this will
// check fail.
void strip_unit_commodities( UnitsState const& units_state,
                             Unit& unit, Colony& colony );

void move_unit_to_colony( UnitsState& units_state,
                          Colony& colony, UnitId unit_id,
                          ColonyJob_t const& job );

// This will put the unit on the map at the location of the
// colony.
void remove_unit_from_colony( UnitsState& units_state,
                              Colony& colony, UnitId unit_id );

} // namespace rn
