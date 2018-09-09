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

#include "core-config.hpp"

#include "id.hpp"
#include "non-copyable.hpp"

#include <variant>
#include <vector>

namespace rn {

// temporary dummy
struct Commodity {
  bool operator==( Commodity const& ) const { return true; }
};

// This Cargo element represents something that may occupy one or
// more cargo slots. E.g., it may represent a unit that takes six
// cargo slots.
using Cargo = std::variant<
  UnitId,
  Commodity
>;

class CargoHold : public movable_only {

public:
  CargoHold( int slots ) : items_{}, slots_( slots ) {}

  ~CargoHold();

  CargoHold( CargoHold&& ) = default;
  CargoHold& operator=( CargoHold&& ) = delete;

  // May not need non-const iterators here.

  //using iterator = std::vector<Cargo>::iterator;
  using const_iterator = std::vector<Cargo>::const_iterator;

  //iterator begin() { return items_.begin(); }
  const_iterator cbegin() const { return items_.cbegin(); }
  //iterator end() { return items_.end(); }
  const_iterator cend() const { return items_.cend(); }

  int slots_occupied() const;
  int slots_remaining() const;
  int slots_total() const;

  //Cargo const& operator[]( int idx ) const;

  bool fits( Cargo cargo ) const;

private:
  // This is the only function that should be called to add some-
  // thing to the cargo.
  friend void ownership_change_to_cargo(
      UnitId new_holder, UnitId held );

  // Caller is expected to first check if these operations will
  // succeed; if they don't succed then an error will be thrown.
  // These functions are only supposed to be called by the
  // friends in the ownership module.
  void add( Cargo cargo );
  void remove( Cargo cargo );

  // This vector will be of variable length, even for a given
  // unit where the number of cargo slots is known. That is be-
  // cause this vector holds one item for each thin in the cargo,
  // and the max size depends on how many slots each individual
  // item takes up. However, the bounds on the size will be:
  // [0, desc->cargo-slots] inclusive.
  std::vector<Cargo> items_;
  int const slots_;
};

} // namespace rn
