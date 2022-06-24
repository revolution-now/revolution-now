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
#include "commodity.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "macros.hpp"
#include "unit-id.hpp"
#include "util.hpp"
#include "variant.hpp"

// Rds
#include "cargo.rds.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/variant.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <vector>

using CargoSlotIndex = int;

// Friends.
namespace rn {

struct UnitsState;

void add_commodity_to_cargo( Commodity const& comm,
                             UnitId holder, int slot,
                             bool try_other_slots );

Commodity rm_commodity_from_cargo( UnitId holder, int slot );

} // namespace rn

namespace rn {

class CargoHold {
 public:
  CargoHold() = default; // for serialization framework.

  explicit CargoHold( int num_slots );

  bool operator==( CargoHold const& ) const = default;

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

  auto begin() const { return o_.slots.begin(); }
  auto end() const { return o_.slots.end(); }

  maybe<CargoSlot_t const&> at( int slot ) const;

  CargoSlot_t const&              operator[]( int idx ) const;
  std::vector<CargoSlot_t> const& slots() const {
    return o_.slots;
  }

  template<typename T>
  maybe<T const&> slot_holds_cargo_type( int idx ) const;

  // If there is a cargo item whose first (and possibly only)
  // slot is `idx`, it will be returned.
  maybe<Cargo_t const&> cargo_starting_at_slot( int idx ) const;
  // If there is a cargo item that occupies the given slot either
  // as its first slot or subsequent slot, it will be returned,
  // alon with its first slot.
  maybe<std::pair<Cargo_t const&, int>> cargo_covering_slot(
      int idx ) const;

  // If unit is in cargo, returns its slot index.
  maybe<int> find_unit( UnitId id ) const;
  // Returns all units in the cargo.
  std::vector<UnitId> units() const;
  // Returns all commodities (and slot indices) in the cargo un-
  // less a specific type is specified in which case it will be
  // limited to those.
  std::vector<std::pair<Commodity, int>> commodities(
      maybe<e_commodity> type = nothing ) const;

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
  ND bool fits( UnitsState const& units_state,
                Cargo_t const& cargo, int slot ) const;

  // Precondition: there must be a cargo item whose first slot is
  // the given slot; if not, then an error will be thrown. This
  // function will determine whether the given cargo item would
  // fit in the cargo if the item at the given slot were first
  // removed. Will not throw an error if the cargo represents a
  // unit that is already in the cargo.
  ND bool fits_with_item_removed(
      UnitsState const& units_state, Cargo_t const& cargo,
      CargoSlotIndex remove_slot,
      CargoSlotIndex insert_slot ) const;

  // Same as above except it will try the entire cargo.
  ND bool fits_somewhere_with_item_removed(
      UnitsState const& units_state, Cargo_t const& cargo,
      int remove_slot, int starting_slot = 0 ) const;

  // Will search through the cargo slots, starting at the speci-
  // fied slot, until one is found at which the given cargo can
  // be inserted, or for commodities, if it can be distributed
  // among multiple slots. If none is found or if the commodity
  // cannot be distributed then it returns false. If an attempt
  // is made to add a unit that is already in the cargo then an
  // exception will be thrown, since this likely reflects a logic
  // error on the part of the caller.
  ND bool fits_somewhere( UnitsState const& units_state,
                          Cargo_t const&    cargo,
                          int starting_slot = 0 ) const;

  // Optimizes the arrangement of cargo items. Places units occu-
  // pying multiple slots further to the left and will consoli-
  // date like commodities where possible. Units with equal occu-
  // pancy size will be put in order of creation, with older
  // units first.
  void compactify( UnitsState const& units_state );

  // Implement refl::WrapsReflected.
  CargoHold( wrapped::CargoHold&& o ) : o_( std::move( o ) ) {}
  wrapped::CargoHold const&         refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "CargoHold";

  // These are to be called after all game state has been loaded,
  // because they need access to units. The wrapped (reflected)
  // object will do some validation that does not require that
  // access, and that is called by this method before it starts
  // its own validation.
  valid_or<generic_err> validate(
      UnitsState const& units_state ) const;
  void validate_or_die( UnitsState const& units_state ) const;

 protected:
  // These friend classes/functions are the only ones that should
  // be allowed to add or remove units to/from the cargo.
  friend struct UnitsState;

  // These are the only functions that should be allowed to add
  // or remove commodities to/from the cargo.
  friend void add_commodity_to_cargo( UnitsState& units_state,
                                      Commodity const& comm,
                                      UnitId holder, int slot,
                                      bool try_other_slots );

  friend Commodity rm_commodity_from_cargo(
      UnitsState& units_state, UnitId holder, int slot );
  // ------------------------------------------------------------

  // Will search through the cargo slots, starting at the speci-
  // fied slot, until one is found at which the given cargo can
  // be inserted. Note that for commodities, this may mean that
  // it gets broken up across a few slots. If none is found or if
  // the commodity cannot be distributed then it returns false.
  // If an attempt is made to add a unit that is already in the
  // cargo then an exception will be thrown, since this likely
  // reflects a logic error on the part of the caller.
  ND bool try_add_somewhere( UnitsState const& units_state,
                             Cargo_t const&    cargo,
                             int starting_slot = 0 );

  // Add the cargo item into the given slot index. Returns true
  // if there was enough space at the given slot to add the
  // cargo, and returns false otherwise. Note that this will not
  // distribute a commodity across multiple slots. If an attempt
  // is made to add a unit that is already in the cargo then an
  // exception will be thrown, since this likely reflects a logic
  // error on the part of the caller.
  ND bool try_add( UnitsState const& units_state,
                   Cargo_t const& cargo, int slot );

  // There must be a cargo item in that slot, i.e., it cannot be
  // `overflow` or `empty`. Otherwise an error will be thrown.
  void remove( int slot );

  // Remove all cargo elements, keeping cargo the same size but
  // filled with "empty"s.
  void clear();

  CargoSlot_t& operator[]( int idx );

  wrapped::CargoHold o_;
};
NOTHROW_MOVE( CargoHold );

// For convenience. Iterates over all cargo items of a certain
// type.
template<typename T>
std::vector<T> CargoHold::items_of_type() const {
  std::vector<T> res;
  for( auto const& slot : o_.slots ) {
    if( auto* cargo = std::get_if<CargoSlot::cargo>( &slot ) )
      if( auto* val = std::get_if<T>( &( cargo->contents ) ) )
        res.emplace_back( *val );
  }
  return res;
}

template<typename T>
int CargoHold::count_items_of_type() const {
  int count = 0;
  for( auto const& slot : o_.slots ) {
    if( auto* cargo = std::get_if<CargoSlot::cargo>( &slot ) )
      if( auto* val = std::get_if<T>( &( cargo->contents ) ) )
        ++count;
  }
  return count;
}

template<typename T>
maybe<T const&> CargoHold::slot_holds_cargo_type(
    int idx ) const {
  CHECK( idx >= 0 && idx < slots_total() );
  return o_.slots[idx]
      .get_if<CargoSlot::cargo>()
      .member( &CargoSlot::cargo::contents )
      .get_if<T>();
}

} // namespace rn
