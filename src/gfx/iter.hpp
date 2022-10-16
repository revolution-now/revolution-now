/****************************************************************
**iter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Useful generators.
*
*****************************************************************/
#pragma once

// gfx
#include "cartesian.hpp"
#include "coord.hpp"

// base
#include "base/generator.hpp"

namespace gfx {

// These will divide the inside of the rect into subrects of the
// given delta and will return an object which, when iterated
// over, will yield those rects. If the rect size is not an even
// multiple of the chunks then some subrects may be smaller than
// the chunk size. In other words, all subrects generated will be
// inside the main rect.
base::generator<rn::Rect> subrects( rn::Rect  rect,
                                    rn::Delta chunk = rn::Delta{
                                        .w = 1, .h = 1 } );

base::generator<rect> subrects( rect rect,
                                size chunk = size{ .w = 1,
                                                   .h = 1 } );

} // namespace gfx
