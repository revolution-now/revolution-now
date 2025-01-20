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
Spreads compute_icon_spread( SpreadSpecs const& specs );

// Applies some heuristics to decide if the spread is such that
// it would be difficult for the player to read the count visu-
// ally, then we put a label on it. Note that in some cases this
// method will either not be called or its result will be over-
// ridden, e.g. in cases such as the colony view when the 'n' key
// is pressed to enumerate all spreads unconditionally.
[[nodiscard]] bool requires_label( Spread const& spread );

/****************************************************************
** Spreads of Tiles.
*****************************************************************/
[[nodiscard]] TileSpread rendered_tile_spread(
    TileSpreadSpecs const& tile_spreads );

void draw_rendered_icon_spread( rr::Renderer& renderer,
                                gfx::point origin,
                                TileSpread const& plan );

} // namespace rn
