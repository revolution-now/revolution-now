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

ND bool square_exists( Y y, X x );

ND Square const& square_at( Y y, X x );
ND Square const& square_at( Coord coord );
ND OptSquareCRef square_at_safe( Y y, X x );

ND SquareSurround surrounding( Y y, X x );

} // namespace rn

// Here  we  open up the std namespace to add a hash function
// spe- cialization for a Coord.
namespace std {
template<>
struct hash<::rn::Coord> {
  auto operator()( ::rn::Coord const& c ) const noexcept {
    using integral_t = decltype( c.x._ );
    // This should be a number that is larger than the width of
    // the world (in tiles) would ever be, that way our mul/add
    // expression below maps each unique map tile onto a unique
    // integer for better hashing. In addition, this is a prime
    // number, since that just seems like a good idea for some
    // reason... not sure why, just a feeling.
    constexpr integral_t prime = 13007;
    // This formula will yield a unique integer for each pos-
    // sible square coordinate in the world, assuming that the
    // world is of a maximum size.
    return hash<integral_t>{}( c.y._ * prime + c.x._ );
  }
};
} // namespace std
