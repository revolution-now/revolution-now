/****************************************************************
**disband.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-09.
*
* Description: Things related to disbanding units.
*
*****************************************************************/
#pragma once

// rds
#include "disband.rds.hpp"

// Revolution Now
#include "wait.hpp"

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

struct SS;
struct SSConst;
struct TS;

enum class e_nation;

DisbandingPermissions disbandable_entities_on_tile(
    SSConst const& ss, e_nation const player_nation,
    gfx::point const tile );

wait<EntitiesOnTile> disband_tile_ui_interaction(
    SSConst const& ss, TS& ts,
    DisbandingPermissions const& disbandable_units );

void execute_disband( SS& ss, TS& ts,
                      EntitiesOnTile const& units );

} // namespace rn
