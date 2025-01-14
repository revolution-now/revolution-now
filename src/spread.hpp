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

/****************************************************************
** Spreads of Tiles.
*****************************************************************/
[[nodiscard]] TileSpreadRenderPlan rendered_tile_spread(
    TileSpreads const& tile_spreads );

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlan const& plan );

} // namespace rn
