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
#include "non-copyable.hpp"

#include <variant>
#include <vector>

namespace rn {

// temporary dummy
struct Commodity {};

// This Cargo element represents something that may occupy one or
// more cargo slots. E.g., it may represent a unit that takes six
// cargo slots.
using Cargo = std::variant<
  UnitId,
  Commodity
>;

class CargoHold : public movable_only {

public:
  CargoHold( int points ) : points_( points ) {}

  CargoHold( CargoHold&& ) = default;
  CargoHold& operator=( CargoHold&& ) = delete;

private:
  // This vector will be of variable length, even for a given
  // unit where the number of cargo slots is known. That is be-
  // cause this vector holds one item for each thin in the cargo,
  // and the max size depends on how many slots each individual
  // item takes up. However, the bounds on the size will be:
  // [0, desc->cargo-slots] inclusive.
  std::vector<Cargo> slots_;
  int const points_;
};

} // namespace rn
