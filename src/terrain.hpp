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

using SquareRef     = std::reference_wrapper<Square>;
using SquareCRef    = std::reference_wrapper<Square const>;
using OptSquareRef  = std::optional<SquareRef>;
using OptSquareCRef = std::optional<SquareCRef>;

struct ND SquareSurround {
  OptSquareCRef north;
  OptSquareCRef south;
  OptSquareCRef east;
  OptSquareCRef west;
};

ND Delta world_size_tiles();
ND Delta world_size_pixels();

ND W world_size_tiles_x();
ND H world_size_tiles_y();
ND W world_size_pixels_x();
ND H world_size_pixels_y();

ND Rect world_rect();
ND Rect world_rect_pixels();

ND bool square_exists( Y y, X x );

ND Square const& square_at( Y y, X x );
ND Square const& square_at( Coord coord );
ND OptSquareCRef square_at_safe( Y y, X x );

ND SquareSurround surrounding( Y y, X x );

} // namespace rn
