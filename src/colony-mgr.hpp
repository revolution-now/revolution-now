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

struct SS;
struct SSConst;
struct TS;

struct ColoniesState;
struct Colony;
struct IGui;
struct IMapUpdater;
struct LandViewPlane;
struct Planes;
struct Player;
struct SettingsState;
struct TerrainState;
struct Unit;
struct UnitsState;

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, std::string_view name );

valid_or<e_found_colony_err> unit_can_found_colony(
    SSConst const& ss, TS& ts, UnitId founder );

// This will change the nation of the colony and all units that
// are workers in the colony as well as units that are in the
// same map square as the colony.
void change_colony_nation( Colony&     colony,
                           UnitsState& units_state,
                           e_nation    new_nation );

// Before calling this, it should already have been the case that
// `can_found_colony` was called to validate; so it should work,
// and thus if it doesn't, it will check-fail.
ColonyId found_colony( SS& ss, TS& ts, UnitId founder,
                       std::string_view name );

// Evolve the colony by one turn.
wait<> evolve_colonies_for_player(
    LandViewPlane& land_view_plane,
    ColoniesState& colonies_state, SettingsState const& settings,
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, IMapUpdater& map_updater, IGui& gui,
    Planes& planes );

// This basically creates a default-constructed colony and gives
// it a nation, name, and location, but nothing more. So it is
// not a valid colony yet. Normal game code shouldn't really call
// this, it is mostly exposed for testing.
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_nation nation, Coord where,
                              std::string_view name );

// Will strip the unit of any commodities (including inventory
// and modifiers) and deposit the commodities into the colony.
void strip_unit_commodities( Unit& unit, Colony& colony );

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

std::vector<UnitId> colony_units_all( Colony const& colony );

bool colony_has_unit( Colony const& colony, UnitId id );

} // namespace rn
