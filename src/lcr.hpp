/****************************************************************
**lcr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-19.
*
* Description: All things related to Lost City Rumors.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "map-updater.hpp"
#include "player.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

namespace rn {

struct TerrainState;
struct UnitsState;
struct EventsState;

bool has_lost_city_rumor( TerrainState const& terrain_state,
                          Coord               square );

enum class e_lost_city_rumor_result { unit_alive, unit_lost };

// Runs through the actions that result from entering a lost city
// rumor, including showing any relevant UI messages, randomly
// choosing the rumor outcome, and making changes to game state
// accordingly. Note that this function may, as one of the rumor
// outcomes, cause the unit to be deleted. If that happens then
// this function will actually delete the unit and will then re-
// turn true. Otherwise returns false.
wait<e_lost_city_rumor_result> enter_lost_city_rumor(
    TerrainState const& terrain_state, UnitsState& units_state,
    EventsState const& events_state, Player& player,
    IMapUpdater& map_updater, UnitId unit_id,
    Coord world_square );

} // namespace rn
