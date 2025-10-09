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
GotoPath compute_goto_path( IGotoMapViewer const& viewer,
                            gfx::point src, gfx::point dst );

// The path contained therein will be empty if no path was found.
GotoPath compute_harbor_goto_path( IGotoMapViewer const& viewer,
                                   gfx::point src );

// This will return false if the unit does not have goto orders.
// If it does have goto orders then it will return true if the
// unit is in the place where it is supposed to go, which could
// be a map tile, high seas, etc.
[[nodiscard]] bool unit_has_reached_goto_target(
    SSConst const& ss, Unit const& unit );

GotoPort find_goto_port( SSConst const& ss,
                         TerrainConnectivity const& connectivity,
                         e_player player_type,
                         e_unit_type unit_type, gfx::point src );

wait<maybe<goto_target>> ask_goto_port(
    SSConst const& ss, IGui& gui, Player const& player,
    GotoPort const& goto_port );

} // namespace rn
