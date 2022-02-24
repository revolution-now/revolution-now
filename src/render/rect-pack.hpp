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

// base
#include "base/expect.hpp"

// C++ standard library
#include <span>

namespace rr {

// TODO: move this out of there.
struct size {
  int w = 0;
  int h = 0;
};

// TODO: move this out of there.
struct point {
  int x = 0;
  int y = 0;
};

// TODO: move this out of there.
struct rect {
  point o = {}; // upper left.
  size  s = {};
};

struct rect_packing {
  size  input     = {};
  point placement = {};
};

// Populates the `placement` for each rect_packing into a box of
// the given width. If successful, it will return the minimum
// height of the box that it used/needed
base::expect<int> pack_rects( std::span<rect_packing> rp,
                              int                     width );

} // namespace rr
