/****************************************************************
**cargo.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles the cargo that a unit may carry.
*
*****************************************************************/
#include "cargo.hpp"

// Revolution Now
#include "errors.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "scope-exit.hpp"
#include "util.hpp"

// Flatbuffers
#include "fb/cargo_generated.h"

// base-util
#include "base-util/algo.hpp"
#include "base-util/variant.hpp"

// Abseil
#include "absl/strings/str_replace.h"

// Range-v3
#include "range/v3/numeric/accumulate.hpp"
#include "range/v3/view/cycle.hpp"
#include "range/v3/view/drop.hpp"
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/group_by.hpp"
#include "range/v3/view/iota.hpp"
#include "range/v3/view/map.hpp"
#include "range/v3/view/take.hpp"

// C++ standard library
#include <type_traits>

using namespace std;

namespace rn {

using util::holds;

namespace {

constexpr int const k_max_commodity_cargo_per_slot = 100;

} // namespace

serial::ReturnValue<FBOffset<fb::CargoSlot>> serialize(
    FBBuilder& builder, CargoSlot_t const& o,
    ::rn::rn_adl_tag ) {
  int32_t       unit_id = 0;
  fb::Commodity commodity;
  // If this is not a commodity slot then just leave this as
  // nullptr so that it doesn't get serialized.
  fb::Commodity*            p_commodity = nullptr;
  fb::e_cargo_slot_contents which =
      fb::e_cargo_slot_contents::empty;
  switch_( o ) {
    case_( CargoSlot::empty ) which =
        fb::e_cargo_slot_contents::empty;
    case_( CargoSlot::overflow ) which =
        fb::e_cargo_slot_contents::overflow;
    case_( CargoSlot::cargo, contents ) {
      switch_( contents ) {
        case_( UnitId ) {
          which   = fb::e_cargo_slot_contents::cargo_unit;
          unit_id = val._;
        }
        case_( Commodity ) {
          which     = fb::e_cargo_slot_contents::cargo_commodity;
          commodity = val.serialize_struct( builder );
          p_commodity = &commodity;
        }
        switch_exhaustive;
      }
    }
    switch_exhaustive;
  }
  return serial::ReturnValue{fb::CreateCargoSlot(
      builder, which, unit_id, p_commodity )};
}

string CargoHold::debug_string() const {
  return absl::StrReplaceAll(
      fmt::format( "{}", FmtJsonStyleList{slots_} ),
      {{"CargoSlot::", ""}} );
}

void CargoHold::check_invariants() const {
  // 0. Accurate reporting of number of slots.
  CHECK( slots_total() == int( slots_.size() ) );
  // 1. First slot is not an `overflow`.
  if( slots_.size() > 0 )
    CHECK( !holds<CargoSlot::overflow>( slots_[0] ) );
  // 2. There are no `overflow`s following `empty`s.
  for( int i = 0; i < slots_total() - 1; ++i )
    if( holds<CargoSlot::empty>( slots_[i] ) )
      CHECK( !holds<CargoSlot::overflow>( slots_[i + 1] ) );
  // 3. There are no `overflow`s following `commodity`s.
  for( int i = 0; i < slots_total() - 1; ++i ) {
    if_v( slots_[i], CargoSlot::cargo, cargo ) {
      if( holds<Commodity>( cargo->contents ) )
        CHECK( !holds<CargoSlot::overflow>( slots_[i + 1] ) );
    }
  }
  // 4. Commodities don't exceed max quantity and are not zero
  // quantity.
  for( auto const& slot : slots_ ) {
    if_v( slot, CargoSlot::cargo, cargo ) {
      if_v( cargo->contents, Commodity, commodity ) {
        CHECK( commodity->quantity <=
               k_max_commodity_cargo_per_slot );
        CHECK( commodity->quantity > 0 );
      }
    }
  }
  // 5. Units with overflow are properly followed by `overflow`.
  for( int i = 0; i < slots_total(); ++i ) {
    auto const& slot = slots_[i];
    if_v( slot, CargoSlot::cargo, cargo ) {
      if_v( cargo->contents, UnitId, unit_id ) {
        auto const& unit = unit_from_id( *unit_id );
        auto        occupies =
            unit.desc().cargo_slots_occupies.value_or( 0 );
        CHECK( occupies > 0 );
        // Check for overflow slots.
        while( occupies > 1 ) {
          --occupies;
          ++i;
          CHECK( i < slots_total() );
          CHECK( holds<CargoSlot::overflow>( slots_[i] ) );
        }
      }
    }
  }
  // 6. Slots occupied matches real contents.
  int occupied = 0;
  for( int i = 0; i < slots_total(); ++i ) {
    auto const& slot = slots_[i];
    switch_( slot ) {
      case_( CargoSlot::empty ) {}
      case_( CargoSlot::overflow ) {}
      case_( CargoSlot::cargo, contents ) {
        switch_( contents ) {
          case_( UnitId ) {
            occupied += unit_from_id( val )
                            .desc()
                            .cargo_slots_occupies.value_or( 0 );
          }
          case_( Commodity ) { occupied++; }
          switch_exhaustive;
        }
      }
      switch_exhaustive;
    }
  }
  CHECK( occupied == slots_occupied() );
}

CargoHold::~CargoHold() {
  if( count_items() != 0 )
    lg.warn( "CargoHold destroyed with {} remaining items.",
             count_items() );
}

int CargoHold::max_commodity_per_cargo_slot() const {
  return k_max_commodity_cargo_per_slot;
}

int CargoHold::slots_occupied() const {
  return slots_total() - slots_remaining();
}

int CargoHold::slots_remaining() const {
  return util::count_if( slots_,
                         L( holds<CargoSlot::empty>( _ ) ) );
}

int CargoHold::slots_total() const { return slots_.size(); }

int CargoHold::count_items() const {
  return util::count_if( slots_,
                         L( holds<CargoSlot::cargo>( _ ) ) );
}

Opt<CRef<CargoSlot_t>> CargoHold::at( int slot ) const {
  if( slot < 0 || slot >= slots_total() ) return nullopt;
  return ( *this )[slot];
}

Opt<CRef<CargoSlot_t>> CargoHold::at(
    CargoSlotIndex slot ) const {
  return this->at( slot._ );
}

CargoSlot_t const& CargoHold::operator[]( int idx ) const {
  CHECK( idx >= 0 && idx < int( slots_.size() ) );
  return slots_[idx];
}

CargoSlot_t& CargoHold::operator[]( int idx ) {
  CHECK( idx >= 0 && idx < int( slots_.size() ) );
  return slots_[idx];
}

CargoSlot_t const& CargoHold::operator[](
    CargoSlotIndex idx ) const {
  return this->operator[]( idx._ );
}

Opt<int> CargoHold::find_unit( UnitId id ) const {
  auto is_unit = [id]( CargoSlot_t const& slot ) {
    if_v( slot, CargoSlot::cargo, cargo ) {
      if_v( cargo->contents, UnitId, unit_id ) {
        return id == *unit_id;
      }
    }
    return false;
  };
  return util::find_first_if( slots_, is_unit );
}

// Returns all units in the cargo.
Vec<UnitId> CargoHold::units() const {
  Vec<UnitId> res;
  for( auto unit_id : items_of_type<UnitId>() )
    res.push_back( unit_id );
  return res;
}

// Returns all commodities (and slot indices) in the cargo un-
// less a specific type is specified in which case it will be
// limited to those.
Vec<Pair<Commodity, int>> CargoHold::commodities(
    Opt<e_commodity> type ) const {
  Vec<Pair<Commodity, int>> res;

  for( auto const& [idx, slot] : rv::enumerate( slots_ ) ) {
    if_v( slot, CargoSlot::cargo, cargo ) {
      if_v( cargo->contents, Commodity, commodity ) {
        if( !type || ( commodity->type == *type ) )
          res.emplace_back( *commodity, idx );
      }
    }
  }
  return res;
}

void CargoHold::compactify() {
  auto           unit_ids    = units();
  auto           comms_pairs = commodities();
  Vec<Commodity> comms       = comms_pairs | rv::keys;
  // First sort by ID, then do a stable sort on slot occupancy to
  // get "deterministic" results.
  util::sort_by_key( unit_ids, []( auto id ) { return id._; } );
  // Negative to do reverse sort.
  util::stable_sort_by_key(
      unit_ids,
      L( -unit_from_id( _ ).desc().cargo_slots_occupies.value_or(
          0 ) ) );
  util::sort_by_key( comms, L( _.type ) );
  clear();
  check_invariants();
  for( UnitId id : unit_ids ) CHECK( try_add_somewhere( id ) );
  auto like_types =
      comms | rv::group_by( L2( _1.type == _2.type ) );
  for( auto group : like_types ) {
    auto           type = group.begin()->type;
    Vec<Commodity> new_comms;
    new_comms.push_back( Commodity{/*type=*/type,
                                   /*quantity=*/0} );
    for( Commodity const& comm : group ) {
      auto delta = k_max_commodity_cargo_per_slot -
                   ( new_comms.back().quantity + comm.quantity );
      if( delta >= 0 )
        new_comms.back().quantity += comm.quantity;
      else {
        new_comms.back().quantity =
            k_max_commodity_cargo_per_slot;
        new_comms.push_back( Commodity{/*type=*/type,
                                       /*quantity=*/-delta} );
      }
    }
    for( auto const& comm : new_comms )
      CHECK( try_add_somewhere( comm ) );
  }
  check_invariants();
}

int CargoHold::max_commodity_quantity_that_fits(
    e_commodity type ) const {
  auto one_slot = variant_function_c( slot, ->, int ) {
    case_( CargoSlot::empty ) {
      return k_max_commodity_cargo_per_slot;
    }
    case_( CargoSlot::overflow ) return 0;
    case_( CargoSlot::cargo ) {
      return matcher_( slot.contents ) {
        case_( UnitId ) return 0;
        case_( Commodity ) {
          if( val.type == type )
            return k_max_commodity_cargo_per_slot - val.quantity;
          return 0;
        }
        matcher_exhaustive;
      }
    }
    variant_function_exhaustive;
  };
  return rg::accumulate( slots_ | rv::transform( one_slot ), 0 );
}

bool CargoHold::fits( Cargo const& cargo, int slot ) const {
  CHECK( slot >= 0 && slot < int( slots_.size() ) );
  return matcher_( cargo ) {
    case_( UnitId ) {
      auto maybe_occupied =
          unit_from_id( val ).desc().cargo_slots_occupies;
      if( !maybe_occupied )
        // Unit cannot be held as cargo.
        result_ false;
      auto occupied = *maybe_occupied;
      // Check that all needed slots are `empty`.
      for( int i = slot; i < slot + occupied; ++i ) {
        if( i >= slots_total() )
          // Not enough slots left.
          result_ false;
        if( !holds<CargoSlot::empty>( slots_[i] ) )
          // Needed slots are not empty.
          result_ false;
      }
      result_ true;
    }
    case_( Commodity ) {
      auto const& proposed = val;
      if( proposed.quantity > k_max_commodity_cargo_per_slot )
        result_ false;
      if( proposed.quantity == 0 ) //
        result_ false;
      result_ matcher_( slots_[slot] ) {
        case_( CargoSlot::overflow ) result_ false;
        case_( CargoSlot::empty ) result_ true;
        case_( CargoSlot::cargo ) {
          result_ matcher_( val.contents ) {
            case_( UnitId ) result_ false;
            case_( Commodity ) {
              if( proposed.type != val.type ) //
                result_ false;
              result_( val.quantity + proposed.quantity <=
                       k_max_commodity_cargo_per_slot );
            }
            matcher_exhaustive;
          }
        }
        matcher_exhaustive;
      }
    }
    matcher_exhaustive;
  }
}

ND bool CargoHold::fits( Cargo const&   cargo,
                         CargoSlotIndex slot ) const {
  return fits( cargo, slot._ );
}

ND bool CargoHold::fits_with_item_removed(
    Cargo const& cargo, CargoSlotIndex remove_slot,
    CargoSlotIndex insert_slot ) const {
  CargoHold new_hold = *this;
  // Need to make sure we clear this out in case the line after
  // throws an exception. This is only needed for convenience
  // when unit testing, which catches exceptions (so it's nice to
  // be exception safe).
  SCOPE_EXIT( new_hold.clear() );
  new_hold.remove( remove_slot._ );
  return new_hold.fits( cargo, insert_slot );
}

bool CargoHold::fits_somewhere( Cargo const& cargo,
                                int starting_slot ) const {
  CargoHold new_hold = *this;
  // Need to make sure we clear this out in case the line after
  // throws an exception. This is only needed for convenience
  // when unit testing, which catches exceptions (so it's nice to
  // be exception safe).
  SCOPE_EXIT( new_hold.clear() );
  auto res = new_hold.try_add_somewhere( cargo, starting_slot );
  return res;
}

bool CargoHold::try_add_somewhere( Cargo const& cargo,
                                   int          starting_from ) {
  if( slots_total() == 0 ) return false;
  CHECK( starting_from >= 0 && starting_from < slots_total() );
  auto slots = rv::ints( 0, slots_total() ) //
               | rv::cycle                  //
               | rv::drop( starting_from )  //
               | rv::take( slots_total() );
  return matcher_( cargo ) {
    case_( UnitId ) {
      for( int idx : slots )
        if( try_add( val, idx ) ) //
          return true;
      return false;
    }
    case_( Commodity ) {
      auto old_slots = slots_;
      auto commodity = val; // make copy.
      CHECK( commodity.quantity > 0 );
      for( int idx : slots ) {
        if( commodity.quantity == 0 ) break;
        switch_( slots_[idx] ) {
          case_( CargoSlot::empty ) {
            auto quantity_to_add =
                std::min( commodity.quantity,
                          k_max_commodity_cargo_per_slot );
            CHECK( quantity_to_add > 0 );
            commodity.quantity -= quantity_to_add;
            CHECK(
                try_add( Commodity{/*type=*/commodity.type,
                                   /*quantity=*/quantity_to_add},
                         idx ),
                "failed to add commodity of type {} and "
                "quantity {} to slot {}",
                commodity.type, quantity_to_add, idx )
          }
          case_( CargoSlot::overflow ) {}
          case_( CargoSlot::cargo ) {
            if_v( val.contents, Commodity, comm_in_slot ) {
              if( comm_in_slot->type == commodity.type ) {
                auto quantity_to_add =
                    std::min( commodity.quantity,
                              k_max_commodity_cargo_per_slot -
                                  comm_in_slot->quantity );
                commodity.quantity -= quantity_to_add;
                if( quantity_to_add > 0 ) {
                  CHECK(
                      try_add( Commodity{
                                   /*type=*/commodity.type,
                                   /*quantity=*/quantity_to_add},
                               idx ),
                      "failed to add commodity of type {} and "
                      "quantity {} to slot {}",
                      commodity.type, quantity_to_add, idx )
                }
              }
            }
          }
          switch_exhaustive;
        }
      }
      if( commodity.quantity == 0 )
        result_ true;
      else {
        // Couldn't make it work, so restore state.
        slots_ = old_slots;
        result_ false;
      }
    }
    matcher_exhaustive;
  }
}

bool CargoHold::try_add( Cargo const& cargo, int slot ) {
  if_v( cargo, UnitId, id ) {
    // Make sure that the unit is not already in this cargo.
    auto units = items_of_type<UnitId>();
    CHECK( util::count_if( units, LC( _ == *id ) ) == 0 );
  }
  if( !fits( cargo, slot ) ) return false;
  // From here on we assume it is totally safe in every way to
  // blindly add this cargo into the given slot(s).
  auto was_added = matcher_( cargo ) {
    case_( UnitId ) {
      auto maybe_occupied =
          unit_from_id( val ).desc().cargo_slots_occupies;
      if( !maybe_occupied ) result_ false;
      auto occupied = *maybe_occupied;
      slots_[slot]  = CargoSlot::cargo{/*contents=*/cargo};
      // Now handle overflow.
      while( slot++, occupied-- > 1 )
        slots_[slot] = CargoSlot::overflow{};
      result_ true;
    }
    case_( Commodity ) {
      if( holds<CargoSlot::empty>( slots_[slot] ) )
        slots_[slot] = CargoSlot::cargo{/*contents=*/cargo};
      else {
        GET_CHECK_VARIANT( cargo, slots_[slot],
                           CargoSlot::cargo );
        GET_CHECK_VARIANT( comm, cargo.contents, Commodity );
        CHECK( comm.type == val.type );
        comm.quantity += val.quantity;
      }
      result_ true;
    }
    matcher_exhaustive;
  }
  check_invariants();
  return was_added;
}

void CargoHold::remove( int slot ) {
  CHECK( slot >= 0 && slot < int( slots_.size() ) );
  CHECK( holds<CargoSlot::cargo>( slots_[slot] ) );
  slots_[slot] = CargoSlot::empty{};
  slot++;
  while( slot < int( slots_.size() ) &&
         holds<CargoSlot::overflow>( slots_[slot] ) ) {
    slots_[slot] = CargoSlot::empty{};
    ++slot;
  }
  check_invariants();
}

void CargoHold::clear() {
  for( auto& slot : slots_ ) //
    slot = CargoSlot::empty{};
  check_invariants();
}

} // namespace rn
