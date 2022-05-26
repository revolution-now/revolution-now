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
#include "igui.hpp"
#include "map-updater.hpp"
#include "player.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// Rds
#include "lcr.rds.hpp"

namespace rn {

struct TerrainState;
struct UnitsState;

bool has_lost_city_rumor( TerrainState const& terrain_state,
                          Coord               square );

e_lcr_explorer_category lcr_explorer_category(
    UnitsState const& units_state, UnitId unit_id );

e_rumor_type pick_rumor_type_result(
    e_lcr_explorer_category explorer, Player const& player );

e_burial_mounds_type pick_burial_mounds_result(
    e_lcr_explorer_category explorer );

bool pick_burial_grounds_result(
    Player const& player, e_lcr_explorer_category explorer,
    e_burial_mounds_type burial_type );

// Given a predetermined rumor type result (and burial mounds re-
// sult, which will only be used in the relevant case) this will
// run through the sequence of actions for that result, including
// showing UI messages, making changes to game state, and poten-
// tially deleting the unit (when that outcome is selected).
//
// This function is the one that should be used for testing be-
// cause it will not manually generate a rumor type result. How-
// ever, it will manually generate gift amounts.
wait<LostCityRumorResult_t> run_lost_city_rumor_result(
    UnitsState& units_state, IGui& gui, Player& player,
    IMapUpdater& map_updater, UnitId unit_id, Coord world_square,
    e_rumor_type type, e_burial_mounds_type burial_type,
    bool has_burial_grounds );

} // namespace rn
