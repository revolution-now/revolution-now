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

// Rds
#include "lcr.rds.hpp"

// Revolution Now
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct IEuroAgent;
struct ILandViewPlane;
struct IMapUpdater;
struct IRand;
struct Player;
struct SS;
struct SSConst;
struct TS;
struct Unit;
struct IMapSearch;

enum class e_unit_type;

[[nodiscard]] LostCityRumor compute_lcr(
    SSConst const& ss, Player const& player, IRand& rand,
    IMapSearch const& map_search, e_unit_type unit_type,
    Coord tile );

// Given a predetermined rumor outcome this will run through the
// sequence of actions for that result, including showing UI mes-
// sages, making changes to game state, and potentially deleting
// the unit (when that outcome is selected).
//
// Note that, although this function will not generate a rumor
// type result (it is provided), it will manually generate gift
// amounts.
wait<LostCityRumorUnitChange> run_lcr(
    SS& ss, ILandViewPlane& land_view, IMapUpdater& map_updater,
    IRand& rand, Player& player, IEuroAgent& agent,
    Unit const& unit, Coord world_square,
    LostCityRumor const& rumor );

} // namespace rn
