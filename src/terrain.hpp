/****************************************************************
**terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Representation of the physical world.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"
#include "geo-types.hpp"

// c++ standard library
#include <functional>
#include <optional>
#include <tuple>

namespace rn {

enum class e_( crust, land, water );

struct ND Square {
  e_crust crust;
};

Delta world_size_tiles();
Delta world_size_pixels();
Rect  world_rect_tiles();
Rect  world_rect_pixels();

bool square_exists( Y y, X x );

Square const&          square_at( Coord coord );
Opt<Ref<Square const>> maybe_square_at( Coord coord );

} // namespace rn
