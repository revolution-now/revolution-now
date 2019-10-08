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
#include "errors.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "macros.hpp"
#include "typed-int.hpp"
#include "util.hpp"

// Flatbuffers
#include "fb/cargo_generated.h"

// base-util
#include "base-util/algo.hpp"
#include "base-util/non-copyable.hpp"
#include "base-util/variant.hpp"

// C++ standard library
#include <variant>
#include <vector>

TYPED_INDEX( CargoSlotIndex );

namespace fb {
struct CargoHold;
}

// Friends.
namespace rn {
void ownership_change_to_cargo( UnitId new_holder, UnitId held,
                                int slot );
void add_commodity_to_cargo( Commodity const& comm,
                             UnitId holder, int slot,
                             bool try_other_slots );
Commodity rm_commodity_from_cargo( UnitId holder, int slot );
namespace internal {
void ownership_disown_unit( UnitId id );
} // namespace internal
} // namespace rn

namespace rn {

using Cargo = std::variant<UnitId, Commodity>;
NOTHROW_MOVE( Cargo );

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
adt_rn( CargoSlot,              //
        ( empty ),              //
        ( overflow ),           //
        ( cargo,                //
          ( Cargo, contents ) ) //
);

serial::ReturnValue<FBOffset<::fb::CargoSlot>> serialize(
    FBBuilder& builder, CargoSlot_t const& o, ::rn::rn_adl_tag );

expect<> deserialize( fb::CargoSlot const* src, CargoSlot_t* dst,
                      ::rn::rn_adl_tag );

class ND CargoHold {
public:
  explicit CargoHold( int num_slots ) : slots_( num_slots ) {
    check_invariants();
  }

  // TODO: eventually we should change this to noexcept(false)
  // and throw an exception from it (after logging an error) if
  // it is called with items in the cargo (probably an indication
  // of a logic error in the program.
  ~CargoHold();

  // We need this so that structures containing this CargoHold
  // can be moved.
  CargoHold( CargoHold&& ) = default;
  CargoHold& operator=( CargoHold&& ) = default;

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

  int max_commodity_per_cargo_slot() const;

  auto begin() const { return slots_.begin(); }
  auto end() const { return slots_.end(); }

  Opt<CRef<CargoSlot_t>> at( int slot ) const;
  Opt<CRef<CargoSlot_t>> at( CargoSlotIndex slot ) const;

  CargoSlot_t const&      operator[]( int idx ) const;
  CargoSlot_t const&      operator[]( CargoSlotIndex idx ) const;
  Vec<CargoSlot_t> const& slots() const { return slots_; }

  template<typename T>
  Opt<CRef<T>> slot_holds_cargo_type( int idx ) const;

  // If unit is in cargo, returns its slot index.
  Opt<int> find_unit( UnitId id ) const;
  // Returns all units in the cargo.
  Vec<UnitId> units() const;
  // Returns all commodities (and slot indices) in the cargo un-
  // less a specific type is specified in which case it will be
  // limited to those.
  Vec<Pair<Commodity, int>> commodities(
      Opt<e_commodity> type = std::nullopt ) const;

  // Find the maximum quantity of the commodity of the given type
  // that can fit in the entire cargo hold (given its current
  // contents). This function will return the quantity of the
  // commodity necessary to 1) fill every empty slot, and 2) fill
  // any partial slot currently occupied by a commodity of the
  // same type.
  int max_commodity_quantity_that_fits( e_commodity type ) const;

  // Checks if the given cargo could be added at the given slot
  // index. If UnitId, will not check for unit id already in
  // cargo.
  ND bool fits( Cargo const& cargo, int slot ) const;
  ND bool fits( Cargo const& cargo, CargoSlotIndex slot ) const;

  // Precondition: there must be a cargo item whose first slot is
  // the given slot; if not, then an error will be thrown. This
  // function will determine whether the given cargo item would
  // fit in the cargo if the item at the given slot were first
  // removed. Will not throw an error if the cargo represents a
  // unit that is already in the cargo.
  ND bool fits_with_item_removed(
      Cargo const& cargo, CargoSlotIndex remove_slot,
      CargoSlotIndex insert_slot ) const;

  // Will search through the cargo slots, starting at the speci-
  // fied slot, until one is found at which the given cargo can
  // be inserted, or for commodities, if it can be distributed
  // among multiple slots. If none is found or if the commodity
  // cannot be distributed then it returns false. If an attempt
  // is made to add a unit that is already in the cargo then an
  // exception will be thrown, since this likely reflects a logic
  // error on the part of the caller.
  ND bool fits_somewhere( Cargo const& cargo,
                          int          starting_slot = 0 ) const;

  // Optimizes the arrangement of cargo items. Places units occu-
  // pying multiple slots further to the left and will consoli-
  // date like commodities where possible. Units with equal occu-
  // pancy size will be put in order of creation, with older
  // units first.
  void compactify();

  std::string debug_string() const;

protected:
  void check_invariants() const;

  // ------------------------------------------------------------
  // These are the only functions that should be allowed to add
  // or remove units to/from the cargo.
  friend void ownership_change_to_cargo( UnitId new_holder,
                                         UnitId held, int slot );
  friend void internal::ownership_disown_unit( UnitId id );

  // These are the only functions that should be allowed to add
  // or remove commodities to/from the cargo.
  friend void add_commodity_to_cargo( Commodity const& comm,
                                      UnitId holder, int slot,
                                      bool try_other_slots );

  friend Commodity rm_commodity_from_cargo( UnitId holder,
                                            int    slot );
  // ------------------------------------------------------------

  // Will search through the cargo slots, starting at the speci-
  // fied slot, until one is found at which the given cargo can
  // be inserted. Note that for commodities, this may mean that
  // it gets broken up across a few slots. If none is found or if
  // the commodity cannot be distributed then it returns false.
  // If an attempt is made to add a unit that is already in the
  // cargo then an exception will be thrown, since this likely
  // reflects a logic error on the part of the caller.
  ND bool try_add_somewhere( Cargo const& cargo,
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

  CargoSlot_t& operator[]( int idx );

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( CargoHold,
  // This will be of fixed length (number of total slots).
  ( std::vector<CargoSlot_t>, slots ));
  // clang-format on

public:
  static expect<> deserialize_table( fb::CargoHold const& src,
                                     CargoHold*           dst );

private:
  CargoHold( CargoHold const& ) = default; // !! default
  CargoHold operator=( CargoHold const& ) = delete;
};
NOTHROW_MOVE( CargoHold );

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

template<typename T>
Opt<CRef<T>> CargoHold::slot_holds_cargo_type( int idx ) const {
  CHECK( idx >= 0 && idx < slots_total() );
  Opt<CRef<T>> res;
  if_v( slots_[idx], CargoSlot::cargo, cargo ) {
    if_v( cargo->contents, T, content ) { //
      res = *content;
    }
  }
  return res;
}

} // namespace rn

DEFINE_FORMAT( rn::CargoHold, "{}", o.debug_string() );
