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

// gfx
#include "gfx/cartesian.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
** General Algos.
*****************************************************************/
IconSpreads compute_icon_spread( IconSpreadSpecs const& specs );

// Applies some heuristics to decide if the spread is such that
// it would be difficult for the player to read the count visu-
// ally, then we put a label on it. Note that in some cases this
// method will either not be called or its result will be over-
// ridden, e.g. in cases such as the colony view when the 'n' key
// is pressed to enumerate all spreads unconditionally.
[[nodiscard]] bool requires_label( IconSpread const& spread );

/****************************************************************
** Spreads of Tiles.
*****************************************************************/
[[nodiscard]] TileSpreadRenderPlan rendered_tile_spread(
    TileSpreads const& tile_spreads );

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlan const& plan );

} // namespace rn
