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

// This is for the case where we just have a single spread that
// will be partially rendered to represent a progress bar. The
// main difference between this algo and the usual spread algo is
// that in this one we relax the requirement that individual
// tiles in a spread must all have the same spacing: non-uniform
// spacings are used to try to fill the entire bounds with the
// tiles, which is better for a progress indicator since that way
// the viewer gets a sense of the progress by comparing the total
// number of rendered tiles against the total bounds, which they
// would not be able to do with the usual spread algo (which
// often settles for using only part of the total bounds).
//
// This will return nothing if it cannot fit within the bounds
// even when all tiles are spaced just one pixel apart.
maybe<ProgressSpread> compute_icon_spread_progress_bar(
    ProgressSpreadSpec const& spec );

// This one is called when we can't fit all the icons within the
// bounds even when all of their spacings are reduced to one
// (i.e., where the principle method above returns nothing). We
// will force it to fit in the bounds by rewriting the counts of
// each spread, and it seems that a good way to do this would be
// to just rewrite all them to be a fraction of the whole in pro-
// portion to their original desired counts.
Spreads compute_icon_spread_proportionate(
    SpreadSpecs const& specs );

// In the case where the tile spread represents a progress indi-
// cator, we may have to adjust the rendered count to represent
// that progress, with special handling of the case when rendered
// count is already less than total count (which can happen when
// the proportional algo is used).
void adjust_rendered_count_for_progress_count(
    SpreadSpec const& spec, Spread& spread, int progress_count );

void adjust_rendered_count_for_progress_count(
    ProgressSpreadSpec const& spec, ProgressSpread& spread,
    int progress_count );

// In the OG sometimes labels can be turned on unconditionally
// (e.g. in the colony view), but even when that does not happen,
// the OG still sometimes puts labels on spreads that it deems to
// be difficult to read, and that's what this function tries to
// replicate by applying some heuristics to decide if the spread
// is such that it would be difficult for the player to read the
// count visually.
[[nodiscard]] bool requires_label( Spread const& spread );

[[nodiscard]] bool requires_label(
    ProgressSpread const& spread );

} // namespace rn
