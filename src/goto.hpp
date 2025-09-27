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

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IGotoMapViewer;
struct SSConst;
struct Unit;

/****************************************************************
** Public API.
*****************************************************************/
base::maybe<GotoPath> compute_goto_path(
    IGotoMapViewer const& viewer, gfx::point src,
    gfx::point dst );

// This will return false if the unit does not have goto orders.
// If it does have goto orders then it will return true if the
// unit is in the place where it is supposed to go, which could
// be a map tile, high seas, etc.
[[nodiscard]] bool unit_has_reached_goto_target(
    SSConst const& ss, Unit const& unit );

} // namespace rn
