/****************************************************************
**image-analysis.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Performs analysis and precomputations on/of images
*              and sprites.
*
*****************************************************************/
#pragma once

// gfx
#include "cartesian.hpp"

namespace gfx {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct image;

/****************************************************************
** Public API.
*****************************************************************/
// Find the minimal bounds inside this rect that contains all
// non-transparent pixels. The returned value has its origin rel-
// ative to the image origin, and will always be contained inside
// the input rect.
[[nodiscard]] rect find_trimmed_bounds_in( image const& img,
                                           rect r );

[[nodiscard]] image compute_burrowed_sprites(
    image const& input, std::vector<rect> const& sprites );

} // namespace gfx
