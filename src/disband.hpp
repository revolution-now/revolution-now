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

namespace rr {
struct ITextometer;
}

namespace rn {

struct IVisibility;
struct Player;
struct SS;
struct SSConst;
struct TS;

DisbandingPermissions disbandable_entities_on_tile(
    SSConst const& ss, IVisibility const& viz,
    gfx::point const tile );

wait<EntitiesOnTile> disband_tile_ui_interaction(
    SSConst const& ss, TS& ts, rr::ITextometer const& textometer,
    Player const& player, IVisibility const& viz,
    DisbandingPermissions const& perms );

wait<> execute_disband( SS& ss, TS& ts, IVisibility const& viz,
                        gfx::point const tile,
                        EntitiesOnTile const& entities );

} // namespace rn
