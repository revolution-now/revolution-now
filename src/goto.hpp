/****************************************************************
**goto.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-25.
*
* Description: Goto-related things.
*
*****************************************************************/
#pragma once

// rds
#include "goto.rds.hpp"

// Revolution Now
#include "maybe.hpp"
#include "wait.hpp"

// ss
#include "ss/goto.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IGotoMapViewer;
struct IGui;
struct IVisibility;
struct GotoRegistry;
struct Player;
struct SSConst;
struct TerrainConnectivity;
struct Unit;

enum class e_player;
enum class e_unit_type;

/****************************************************************
** Public API.
*****************************************************************/
// The path contained therein will be empty if no path was found
// to the destination. Likewise if src == dst.
[[nodiscard]] GotoPath compute_goto_path(
    IGotoMapViewer const& viewer, gfx::point src,
    gfx::point dst );

// The path contained therein will be empty if no path was found.
[[nodiscard]] GotoPath compute_harbor_goto_path(
    IGotoMapViewer const& viewer, gfx::point src );

// Note that the unit_player might not be the same as the player
// associated with the viz object (i.e. the latter might have
// full visibility).
[[nodiscard]] maybe<GotoTargetSnapshot>
compute_goto_target_snapshot( SSConst const& ss,
                              IVisibility const& viz,
                              e_player unit_player,
                              gfx::point tile );

[[nodiscard]] bool is_new_goto_snapshot_allowed(
    maybe<GotoTargetSnapshot> old,
    GotoTargetSnapshot const& New );

// This should always be called to create such a target as it en-
// sures that the snapshot gets populated.
[[nodiscard]] goto_target::map create_goto_map_target(
    SSConst const& ss, e_player unit_player, gfx::point tile );

// Will return true if the unit is in the place where it is sup-
// posed to go, which could be a map tile, high seas, etc. The
// unit does not need to have goto orders necessarily.
[[nodiscard]] bool unit_has_reached_goto_target(
    SSConst const& ss, Unit const& unit,
    goto_target const& target );

[[nodiscard]] GotoPort find_goto_port(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player player_type, e_unit_type unit_type,
    gfx::point src );

wait<maybe<goto_target>> ask_goto_port(
    SSConst const& ss, IGui& gui, Player const& player,
    GotoPort const& goto_port, e_unit_type unit_type );

// Note that the unit does not have to have goto orders per se to
// use this, since it is intended to be used for both human
// players and AI, the latter of which don't use explicit goto
// orders.
[[nodiscard]] EvolveGoto
find_next_move_for_unit_with_goto_target(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    GotoRegistry& registry, IGotoMapViewer const& goto_viewer,
    Unit const& unit, goto_target const& target );

} // namespace rn
