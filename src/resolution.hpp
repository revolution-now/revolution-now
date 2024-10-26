/****************************************************************
**resolution.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-14.
*
* Description: Handles things related to the physical and logical
*              resolutions used by the game.
*
*****************************************************************/
#pragma once

// rds
#include "resolution.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// C++ standard library
#include <vector>

namespace rn {

gfx::ResolutionRatings compute_logical_resolution_ratings(
    gfx::size physical_size );

Resolutions compute_resolutions( gfx::size physical_size );

} // namespace rn
