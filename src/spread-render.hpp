/****************************************************************
**spread-render.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Creates and renders spreads of tiles.
*
*****************************************************************/
#pragma once

// rds
#include "spread-render.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] TileSpreadRenderPlans render_plan_for_tile_spread(
    TileSpreadSpecs const& tile_spreads );

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlan const& plan );

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlans const& plan );

} // namespace rn
