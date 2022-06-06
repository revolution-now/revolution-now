/****************************************************************
**colony-evolve.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-04.
*
* Description: Evolves one colony one turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
// #include "hello.hpp"

// Rds
#include "colony-evolve.rds.hpp"

// base
// #include "base/hello.hpp"

namespace rn {

struct Colony;
struct IGui;
struct IMapUpdater;
struct Player;
struct SettingsState;
struct TerrainState;
struct UnitsState;

// Evolve the colony by one turn. This is not a coroutine because
// we want it to also be used for the AI players.
ColonyEvolution evolve_colony_one_turn(
    Colony& colony, SettingsState const& settings,
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, IMapUpdater& map_updater );

} // namespace rn
