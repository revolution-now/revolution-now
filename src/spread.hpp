/****************************************************************
**spread.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Algorithm for doing the icon spread.
*
*****************************************************************/
#pragma once

// rds
#include "spread.rds.hpp"

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
// This is a general method that just applies the core algorithm,
// as it doesn't know anything about the game's tiles.
Spreads compute_icon_spread( SpreadSpecs const& specs );

// In the OG sometimes labels can be turned on unconditionally
// (e.g. in the colony view), but even when that does not happen,
// the OG still sometimes puts labels on spreads that it deems to
// be difficult to read, and that's what this function tries to
// replicate by applying some heuristics to decide if the spread
// is such that it would be difficult for the player to read the
// count visually.
[[nodiscard]] bool requires_label( Spread const& spread );

} // namespace rn
