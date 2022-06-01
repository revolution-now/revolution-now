/****************************************************************
**turn.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "igui.hpp"
#include "wait.hpp"

// C++ standard library
#include <exception>

namespace rn {

struct game_quit_interrupt : std::exception {};
struct game_load_interrupt : std::exception {};

struct IMapUpdater;
struct SettingsState;
struct PlayersState;
struct UnitsState;
struct TerrainState;

wait<> next_turn( PlayersState&        players_state,
                  TerrainState const&  terrain_state,
                  UnitsState&          units_state,
                  SettingsState const& settings,
                  IMapUpdater& map_updater, IGui& gui );

} // namespace rn
