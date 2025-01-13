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

int spread_width_for_tile( e_tile tile );

IconSpreads compute_icon_spread( IconSpreadSpecs const& specs );

void render_icon_spread( rr::Renderer& renderer,
                         gfx::point where,
                         RenderableIconSpreads const& rspreads );

} // namespace rn
