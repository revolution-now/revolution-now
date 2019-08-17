/****************************************************************
**cargo.hpp
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

// Revolution Now
#include "adt.hpp"
#include "commodity.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "macros.hpp"
#include "util.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/non-copyable.hpp"
#include "base-util/variant.hpp"

// C++ standard library
#include <variant>
#include <vector>

namespace rn {

using Cargo = std::variant<UnitId, Commodity>;
ASSERT_NOTHROW_MOVING( Cargo );

// Here is an example of the way the cargo layout works:
//
// +------------------------------------------------------------+
// |         |         |         |         |         |          |
// |  Comm.  | UnitId  | Overfl. |  Empty  |  Empty  |  UnitId  |
// |         |         |         |         |         |          |
// +------------------------------------------------------------+
//      1        2         3          4         5         6
//
// That means that the first slot is occupied by a commodity, the
// second AND third slots are occupied by a single unit that
// takes up two slots, the fourth slot is empty, the fifth slot
// is empty, and the last slot is occupied by a unit (that takes
// one slot).
//
// NOTE: the `empty` state must be first in the list so that it
// it will be the default-constructed value.
ADT_RN( CargoSlot,              //
        ( empty ),              //
        ( overflow ),           //
        ( cargo,                //
          ( Cargo, contents ) ) //
);

class ND CargoHold : public util::movable_only {
public:
  explicit CargoHold( int num_slots ) : slots_( num_slots ) {
    check_invariants();
  }

  ~CargoHold(); // TODO: can delete this eventually.

  CargoHold( CargoHold&& ) = default;

  int slots_occupied() const;
  int slots_remaining() const;
  int slots_total() const;

  // Total items that are in cargo.
  int count_items() const;

  template<typename T>
  int count_items_of_type() const;

  // For convenience. Iterates over all cargo items of a certain
  // type.
  template<typename T>
  std::vector<T> items_of_type() const;

  auto begin() const { return slots_.begin(); }
  auto end() const { return slots_.end(); }

  CargoSlot_t const&      operator[]( int idx ) const;
  Vec<CargoSlot_t> const& slots() const { return slots_; }

  // If unit is in cargo, returns its slot index.
  Opt<int> find_unit( UnitId id ) const;
  // Returns all units in the cargo.
  Vec<UnitId> units() const;
  // Returns all commodities (and slot indices) in the cargo un-
  // less a specific type is specified in which case it will be
  // limited to those.
  Vec<Pair<Commodity, int>> commodities(
      Opt<e_commodity> type = std::nullopt ) const;

  // Checks if the given cargo could be added at the given slot
  // index. If UnitId, will not check for unit id already in
  // cargo.
  ND bool fits( Cargo const& cargo, int slot ) const;

  // Will search through the cargo slots, starting at the speci-
  // fied slot, until one is found at which the given cargo can
  // be inserted, or for commodities, if it can be distributed
  // among multiple slots. If none is found or if the commodity
  // cannot be distributed then it returns false. If an attempt
  // is made to add a unit that is already in the cargo then an
  // exception will be thrown, since this likely reflects a logic
  // error on the part of the caller.
  ND bool fits_as_available( Cargo const& cargo,
                             int starting_slot = 0 ) const;

  // Optimizes the arrangement of cargo items. Places units occu-
  // pying multiple slots further to the left and will consoli-
  // date like commodities where possible. Units with equal occu-
  // pancy size will be put in order of creation, with older
  // units first.
  void compactify();

  std::string debug_string() const;

protected:
  void check_invariants() const;

  // This is the only function that should be called to add some-
  // thing to the cargo.
  friend void ownership_change_to_cargo( UnitId new_holder,
                                         UnitId held );
  friend void ownership_change_to_cargo( UnitId new_holder,
                                         UnitId held, int slot );
  friend void ownership_disown_unit( UnitId id );

  // Will search through the cargo slots, starting at the speci-
  // fied slot, until one is found at which the given cargo can
  // be inserted. Note that for commodities, this may mean that
  // it gets broken up across a few slots. If none is found or if
  // the commodity cannot be distributed then it returns false.
  // If an attempt is made to add a unit that is already in the
  // cargo then an exception will be thrown, since this likely
  // reflects a logic error on the part of the caller.
  ND bool try_add_as_available( Cargo const& cargo,
                                int          starting_slot = 0 );

  // Add the cargo item into the given slot index. Returns true
  // if there was enough space at the given slot to add the
  // cargo, and returns false otherwise. Note that this will not
  // distribute a commodity across multiple slots. If an attempt
  // is made to add a unit that is already in the cargo then an
  // exception will be thrown, since this likely reflects a logic
  // error on the part of the caller.
  ND bool try_add( Cargo const& cargo, int slot );

  // There must be a cargo item in that slot, i.e., it cannot be
  // `overflow` or `empty`. Otherwise an error will be thrown.
  void remove( int slot );

  // Remove all cargo elements, keeping cargo the same size but
  // filled with "empty"s.
  void clear();

  // This will be of fixed length (number of total slots).
  std::vector<CargoSlot_t> slots_;
};

// For convenience. Iterates over all cargo items of a certain
// type.
template<typename T>
std::vector<T> CargoHold::items_of_type() const {
  std::vector<T> res;
  for( auto const& slot : slots_ ) {
    if_v( slot, CargoSlot::cargo, cargo ) {
      if_v( cargo->contents, T, val ) { //
        res.emplace_back( *val );
      }
    }
  }
  return res;
}

template<typename T>
int CargoHold::count_items_of_type() const {
  int count = 0;
  for( auto const& slot : slots_ ) {
    if_v( slot, CargoSlot::cargo, cargo ) {
      if_v( cargo->contents, T, val ) { //
        ++count;
      }
    }
  }
  return count;
}

} // namespace rn

DEFINE_FORMAT( rn::CargoHold, "{}", o.debug_string() );
