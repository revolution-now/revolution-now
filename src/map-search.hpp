/****************************************************************
**map-search.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Algorithms for searching the map.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/generator.hpp"

namespace rn {

struct SSConst;

// Yields an infinite stream of points spiraling outward from
// the starting point, i.e. (a-y):
//
//   j k l m n
//   y b c d o
//   x i a e p
//   w h g f q
//   v u t s r
//
base::generator<gfx::point> outward_spiral_search(
    gfx::point const start );

// Same as above but will only yield squares that exist on the
// map. It will stop spiral-searching when it has yielded all of
// the points on the map once.
base::generator<gfx::point> outward_spiral_search_existing(
    SSConst const& ss, gfx::point const start );

} // namespace rn
