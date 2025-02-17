/****************************************************************
**spread-algo.hpp
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
#include "spread-algo.rds.hpp"

// Revolution Now
#include "maybe.hpp"

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
// This is a general method that just applies the core algorithm,
// as it doesn't know anything about the game's tiles.
maybe<Spreads> compute_icon_spread( SpreadSpecs const& specs );

// This one is called when we can't fit all the icons within the
// bounds even when all of their spacings are reduced to one
// (i.e., where the principle method above returns nothing). We
// will force it to fit in the bounds by rewriting the counts of
// each spread, and it seems that a good way to do this would be
// to just rewrite all them to be a fraction of the whole in pro-
// portion to their original desired counts.
Spreads compute_icon_spread_proportionate(
    SpreadSpecs const& specs );

void adjust_rendered_count_for_progress_count(
    Spread& spread, int progress_count );

// In the OG sometimes labels can be turned on unconditionally
// (e.g. in the colony view), but even when that does not happen,
// the OG still sometimes puts labels on spreads that it deems to
// be difficult to read, and that's what this function tries to
// replicate by applying some heuristics to decide if the spread
// is such that it would be difficult for the player to read the
// count visually.
[[nodiscard]] bool requires_label( Spread const& spread );

} // namespace rn
