/****************************************************************
* world.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Representation of the physical world.
*
*****************************************************************/

#pragma once

#include <functional>
#include <optional>
#include <tuple>

namespace rn {

struct Square {
  bool land;
};

using SquareRef = std::reference_wrapper<Square>;
using SquareCRef = std::reference_wrapper<Square const>;
using OptSquareRef = std::optional<SquareRef>;
using OptSquareCRef = std::optional<SquareCRef>;

struct SquareSurround {
  OptSquareCRef north;
  OptSquareCRef south;
  OptSquareCRef east;
  OptSquareCRef west;
};

std::tuple<int/*y*/,int/*x*/> world_size_tiles();

bool square_exists( int y, int x );

Square const& square_at( int y, int x );
OptSquareCRef square_at_safe( int y, int x );

SquareSurround surrounding( int y, int x );

} // namespace rn

