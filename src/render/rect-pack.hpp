/****************************************************************
**rect-pack.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-24.
*
* Description: Rect Packer.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <span>

namespace rr {

// This will attempt to pack the rects into an area at most the
// size of max_size. The rects in the input will not be re-
// arranged, they will just have their `origin`s filled out. To
// emphasize, the origins of the rects in the input are ignored
// and overwritten as output; only the sizes are needed as input.
// On success, returns the size actually used to pack all of the
// rects. On failure, some of the origins in the input range may
// still have been partially edited.
base::maybe<gfx::size> pack_rects( std::span<gfx::rect> rp,
                                   gfx::size const max_size );

} // namespace rr
