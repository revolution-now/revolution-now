/****************************************************************
**spread-builder.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Helper for building renderable tile spreads.
*
*****************************************************************/
#pragma once

// rds
#include "spread-builder.rds.hpp"

namespace rr {
struct ITextometer;
}

namespace rn {

TileSpreadRenderPlan build_tile_spread(
    rr::ITextometer const& textometer,
    TileSpreadConfig const& config );

TileSpreadRenderPlan build_progress_tile_spread(
    rr::ITextometer const& textometer,
    ProgressTileSpreadConfig const& config );

TileSpreadRenderPlan build_inhomogenous_tile_spread(
    InhomogeneousTileSpreadConfig const& config );

TileSpreadRenderPlans build_tile_spread_multi(
    rr::ITextometer const& textometer,
    TileSpreadConfigMulti const& configs );

} // namespace rn
