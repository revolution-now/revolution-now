/****************************************************************
* cargo.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles the cargo that a unit may carry.
*
*****************************************************************/
#pragma once

#include "id.hpp"

#include <variant>
#include <vector>

namespace rn {

// temporary dummy
struct Commodity {};

using Cargo = std::variant<
  UnitId,
  Commodity
>;

class CargoHold {

public:
  CargoHold() : slots_{}, points_( 0 ) {}
  CargoHold( int points ) : points_( points ) {}

  CargoHold( CargoHold&& ) = default;
  CargoHold& operator=( CargoHold&& ) = default;

private:
  CargoHold( CargoHold const& ) = delete;
  CargoHold& operator=( CargoHold const& ) = delete;

  // This vector will be of variable length, even for a given
  // unit where the number of cargo slots is known.  That is
  // because this vector holds one item for each thin in the
  // cargo, and the max size depends on how many slots each
  // individual item takes up.  However, the bounds on the size
  // will be [0, desc->cargo-slots] inclusive.
  std::vector<Cargo> slots_;
  int points_;
};

} // namespace rn
