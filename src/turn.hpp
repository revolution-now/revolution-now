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
#include "wait.hpp"

// C++ standard library
#include <exception>

namespace rn {

struct ColoniesState;
struct IMapUpdater;
struct LandViewState;
struct SettingsState;
struct Planes;
struct PlayersState;
struct UnitsState;
struct TerrainState;
struct TurnState;
struct WindowPlane;

wait<> turn_loop( Planes& planes, PlayersState& players_state,
                  TerrainState const&  terrain_state,
                  LandViewState&       land_view_state,
                  UnitsState&          units_state,
                  SettingsState const& settings,
                  TurnState&           turn_state,
                  ColoniesState&       colonies_state,
                  IMapUpdater&         map_updater );

} // namespace rn
