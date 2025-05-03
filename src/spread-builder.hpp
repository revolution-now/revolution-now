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

// This is for when you have a fixed amount of space, and you
// want to create a tile spread within it.
TileSpreadRenderPlan build_tile_spread(
    rr::ITextometer const& textometer,
    TileSpreadConfig const& config );

// Same as above, but allows multiple spreads.
TileSpreadRenderPlans build_tile_spread_multi(
    rr::ITextometer const& textometer,
    TileSpreadConfigMulti const& configs );

// This is for progress bars where the spacing of the tiles needs
// to vary in order to fill a space with a given number of tiles.
TileSpreadRenderPlan build_progress_tile_spread(
    rr::ITextometer const& textometer,
    ProgressTileSpreadConfig const& config );

// There are a fixed number of tiles rendered and there are a
// fixed number of pixels apart.
TileSpreadRenderPlan build_fixed_tile_spread(
    FixedTileSpreadConfig const& config );

// This one is for when you want a spread consisting of different
// tiles. NOTE: this does not work well when there is a large
// variance in the trimmed width of the tiles.
TileSpreadRenderPlan build_inhomogeneous_tile_spread(
    rr::ITextometer const& textometer,
    InhomogeneousTileSpreadConfig const& config );

} // namespace rn
