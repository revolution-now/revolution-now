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

// gfx
#include "gfx/logical.hpp"

// C++ standard library
#include <vector>

namespace rn {

Resolutions compute_resolutions( gfx::Monitor const& monitor,
                                 gfx::size physical_window );

gfx::ResolutionRatingOptions resolution_rating_options();

} // namespace rn
