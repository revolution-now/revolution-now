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

// Revolution Now
#include "maybe.hpp"

// gfx
#include "gfx/aspect.hpp"

// C++ standard library
#include <vector>

namespace rn {

gfx::ResolutionRatings compute_logical_resolution_ratings(
    gfx::size physical_size );

maybe<gfx::Resolution> recompute_best_logical_resolution(
    gfx::size physical_size );

// This is only used if there are no available logical resolu-
// tions and we're forced to put something on the screen.
maybe<gfx::Resolution>
recompute_best_unavailable_logical_resolution(
    gfx::size physical_size );

} // namespace rn
