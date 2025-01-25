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

namespace rn {

TileSpreadRenderPlans build_tile_spread(
    TileSpreadConfigMulti const& configs );

} // namespace rn
