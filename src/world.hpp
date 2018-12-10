/****************************************************************
**world.hpp
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
#include "geo-types.hpp"
#include "global-constants.hpp"

// c++ standard library
#include <functional>
#include <optional>
#include <tuple>

namespace rn {

struct ND Square {
  bool land;
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

ND std::tuple<H, W> world_size_tiles();
ND std::tuple<H, W> world_size_pixels();

ND W world_size_tiles_x();
ND H world_size_tiles_y();
ND W world_size_pixels_x();
ND H world_size_pixels_y();

ND Rect world_rect();

ND bool square_exists( Y y, X x );

ND Square const& square_at( Y y, X x );
ND OptSquareCRef square_at_safe( Y y, X x );

ND SquareSurround surrounding( Y y, X x );

} // namespace rn

// Here  we  open up the std namespace to add a hash function
// spe- cialization for a Coord.
namespace std {
template<>
struct hash<::rn::Coord> {
  auto operator()( ::rn::Coord const& c ) const noexcept {
    // This formula will yield a unique integer for each pos-
    // sible square coordinate in the world, assuming that the
    // world is of a maximum size.
    return hash<decltype( c.x._ )>{}(
        c.y._ * ::rn::g_world_max_width._ + c.x._ );
  }
};
} // namespace std
