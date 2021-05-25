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
#include "error.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "ustate.hpp"
#include "util.hpp"
#include "variant.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"

// Abseil
#include "absl/strings/str_replace.h"

// C++ standard library
#include <type_traits>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

constexpr int const k_max_commodity_cargo_per_slot = 100;

} // namespace

string CargoHold::debug_string() const {
  return absl::StrReplaceAll(
      fmt::format( "{}", FmtJsonStyleList{ slots_ } ),
      { { "CargoSlot::", "" } } );
}

valid_deserial_t CargoHold::check_invariants_post_load() const {
  try {
    check_invariants_or_abort();
    return valid;
  } catch( exception const& e ) {
    return invalid_deserial( fmt::format(
        "CargoHold invariants not upheld: {}", e.what() ) );
  }
}

void CargoHold::check_invariants_or_abort() const {
  CHECK_HAS_VALUE( check_invariants() );
}

valid_or<generic_err> CargoHold::check_invariants() const {
  // 0. Accurate reporting of number of slots.
  TRUE_OR_RETURN_GENERIC_ERR( slots_total() ==
                              int( slots_.size() ) );
  // 1. First slot is not an `overflow`.
  if( slots_.size() > 0 )
    TRUE_OR_RETURN_GENERIC_ERR(
        !holds<CargoSlot::overflow>( slots_[0] ) );
  // 2. There are no `overflow`s following `empty`s.
  for( int i = 0; i < slots_total() - 1; ++i )
    if( holds<CargoSlot::empty>( slots_[i] ) )
      TRUE_OR_RETURN_GENERIC_ERR(
          !holds<CargoSlot::overflow>( slots_[i + 1] ) );
  // 3. There are no `overflow`s following `commodity`s.
  for( int i = 0; i < slots_total() - 1; ++i ) {
    if( auto* cargo = get_if<CargoSlot::cargo>( &slots_[i] ) )
      if( holds<Commodity>( cargo->contents ) )
        TRUE_OR_RETURN_GENERIC_ERR(
            !holds<CargoSlot::overflow>( slots_[i + 1] ) );
  }
  // 4. Commodities don't exceed max quantity and are not zero
  // quantity.
  for( auto const& slot : slots_ ) {
    if( auto* cargo = get_if<CargoSlot::cargo>( &slot ) ) {
      if( auto* commodity =
              get_if<Commodity>( &( cargo->contents ) ) ) {
        TRUE_OR_RETURN_GENERIC_ERR(
            commodity->quantity <=
            k_max_commodity_cargo_per_slot );
        TRUE_OR_RETURN_GENERIC_ERR( commodity->quantity > 0 );
      }
    }
  }
  // 5. Units with overflow are properly followed by `overflow`.
  for( int i = 0; i < slots_total(); ++i ) {
    auto const& slot = slots_[i];
    if( auto* cargo = get_if<CargoSlot::cargo>( &slot ) ) {
      if( auto* unit_id =
              get_if<UnitId>( &( cargo->contents ) ) ) {
        auto const& unit = unit_from_id( *unit_id );
        auto        occupies =
            unit.desc().cargo_slots_occupies.value_or( 0 );
        TRUE_OR_RETURN_GENERIC_ERR( occupies > 0 );
        // Check for overflow slots.
        while( occupies > 1 ) {
          --occupies;
          ++i;
          TRUE_OR_RETURN_GENERIC_ERR( i < slots_total() );
          TRUE_OR_RETURN_GENERIC_ERR(
              holds<CargoSlot::overflow>( slots_[i] ) );
        }
      }
    }
  }
  // 6. Slots occupied matches real contents.
  int occupied = 0;
  for( int i = 0; i < slots_total(); ++i ) {
    auto const& slot = slots_[i];
    switch( slot.to_enum() ) {
      case CargoSlot::e::empty: //
        break;
      case CargoSlot::e::overflow: //
        break;
      case CargoSlot::e::cargo: {
        auto& cargo = slot.get<CargoSlot::cargo>();
        overload_visit(
            cargo.contents,
            [&]( UnitId id ) {
              occupied +=
                  unit_from_id( id )
                      .desc()
                      .cargo_slots_occupies.value_or( 0 );
            },
            [&]( Commodity const& ) { occupied++; } );
        break;
      }
    }
  }
  TRUE_OR_RETURN_GENERIC_ERR( occupied == slots_occupied() );
  return valid;
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

maybe<CargoSlot_t const&> CargoHold::at( int slot ) const {
  if( slot < 0 || slot >= slots_total() ) return nothing;
  return ( *this )[slot];
}

maybe<CargoSlot_t const&> CargoHold::at(
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

maybe<int> CargoHold::find_unit( UnitId id ) const {
  for( size_t idx = 0; idx < slots_.size(); ++idx )
    if( slot_holds_cargo_type<UnitId>( idx ) == id ) //
      return idx;
  return nothing;
}

// Returns all units in the cargo.
vector<UnitId> CargoHold::units() const {
  vector<UnitId> res;
  for( auto unit_id : items_of_type<UnitId>() )
    res.push_back( unit_id );
  return res;
}

// Returns all commodities (and slot indices) in the cargo un-
// less a specific type is specified in which case it will be
// limited to those.
vector<pair<Commodity, int>> CargoHold::commodities(
    maybe<e_commodity> type ) const {
  vector<pair<Commodity, int>> res;

  for( auto const& [idx, slot] :
       rl::all( slots_ ).enumerate() ) {
    if( auto* cargo = get_if<CargoSlot::cargo>( &slot ) )
      if( auto* commodity =
              get_if<Commodity>( &( cargo->contents ) ) )
        if( !type || ( commodity->type == *type ) )
          res.emplace_back( *commodity, idx );
  }
  return res;
}

void CargoHold::compactify() {
  auto unit_ids    = units();
  auto comms_pairs = commodities();
  auto comms       = rl::all( comms_pairs ).keys().to_vector();
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
  check_invariants_or_abort();
  for( UnitId id : unit_ids ) CHECK( try_add_somewhere( id ) );
  auto like_types =
      rl::all( comms ).group_by_L( _1.type == _2.type );
  for( auto group : like_types ) {
    auto              type = group.begin()->type;
    vector<Commodity> new_comms;
    new_comms.push_back(
        Commodity{ /*type=*/type, /*quantity=*/0 } );
    for( Commodity const& comm : group ) {
      auto delta = k_max_commodity_cargo_per_slot -
                   ( new_comms.back().quantity + comm.quantity );
      if( delta >= 0 )
        new_comms.back().quantity += comm.quantity;
      else {
        new_comms.back().quantity =
            k_max_commodity_cargo_per_slot;
        new_comms.push_back(
            Commodity{ /*type=*/type, /*quantity=*/-delta } );
      }
    }
    for( auto const& comm : new_comms )
      CHECK( try_add_somewhere( comm ) );
  }
  check_invariants_or_abort();
}

int CargoHold::max_commodity_quantity_that_fits(
    e_commodity type ) const {
  auto one_slot = [&]( CargoSlot_t const& slot ) {
    switch( slot.to_enum() ) {
      case CargoSlot::e::empty:
        return k_max_commodity_cargo_per_slot;
      case CargoSlot::e::overflow: //
        return 0;
      case CargoSlot::e::cargo: {
        auto& cargo = slot.get<CargoSlot::cargo>();
        return overload_visit(
            cargo.contents, []( UnitId ) { return 0; },
            [&]( Commodity const& c ) {
              return ( c.type == type )
                         ? ( k_max_commodity_cargo_per_slot -
                             c.quantity )
                         : 0;
            } );
      }
    }
  };
  return rl::all( slots_ ).map( one_slot ).accumulate();
}

bool CargoHold::fits( Cargo const& cargo, int slot ) const {
  CHECK( slot >= 0 && slot < int( slots_.size() ) );
  return overload_visit(
      cargo,
      [&]( UnitId id ) {
        auto maybe_occupied =
            unit_from_id( id ).desc().cargo_slots_occupies;
        if( !maybe_occupied )
          // Unit cannot be held as cargo.
          return false;
        auto occupied = *maybe_occupied;
        // Check that all needed slots are `empty`.
        for( int i = slot; i < slot + occupied; ++i ) {
          if( i >= slots_total() )
            // Not enough slots left.
            return false;
          if( !holds<CargoSlot::empty>( slots_[i] ) )
            // Needed slots are not empty.
            return false;
        }
        return true;
      },
      [&]( Commodity const& c ) {
        auto const& proposed = c;
        if( proposed.quantity > k_max_commodity_cargo_per_slot )
          return false;
        if( proposed.quantity == 0 ) //
          return false;
        switch( auto& v = slots_[slot]; v.to_enum() ) {
          case CargoSlot::e::overflow: {
            return false;
          }
          case CargoSlot::e::empty: {
            return true;
          }
          case CargoSlot::e::cargo: {
            auto& cargo = v.get<CargoSlot::cargo>();
            return overload_visit(
                cargo.contents, []( UnitId ) { return false; },
                [&]( Commodity const& c ) {
                  if( proposed.type != c.type ) //
                    return false;
                  return ( c.quantity + proposed.quantity <=
                           k_max_commodity_cargo_per_slot );
                } );
            break;
          }
        }
      } );
}

ND bool CargoHold::fits( Cargo const&   cargo,
                         CargoSlotIndex slot ) const {
  return fits( cargo, slot._ );
}

ND bool CargoHold::fits_with_item_removed(
    Cargo const& cargo, CargoSlotIndex remove_slot,
    CargoSlotIndex insert_slot ) const {
  CargoHold new_hold = *this;
  new_hold.remove( remove_slot._ );
  return new_hold.fits( cargo, insert_slot );
}

ND bool CargoHold::fits_somewhere_with_item_removed(
    Cargo const& cargo, int remove_slot,
    int starting_slot ) const {
  CargoHold new_hold = *this;
  new_hold.remove( remove_slot );
  return new_hold.fits_somewhere( cargo, starting_slot );
}

bool CargoHold::fits_somewhere( Cargo const& cargo,
                                int starting_slot ) const {
  CargoHold new_hold = *this;
  // Do this so that this tmp cargo hold does not get destroyed
  // with stuff in it, which currently triggers a warning to be
  // logged to the console and slows things down.
  SCOPE_EXIT( new_hold.slots_.clear() );
  return new_hold.try_add_somewhere( cargo, starting_slot );
}

bool CargoHold::try_add_somewhere( Cargo const& cargo,
                                   int          starting_from ) {
  if( slots_total() == 0 ) return false;
  CHECK( starting_from >= 0 && starting_from < slots_total() );
  auto slots = rl::ints( 0, slots_total() )
                   .cycle()
                   .drop( starting_from )
                   .take( slots_total() );
  return overload_visit(
      cargo,
      [&]( UnitId id ) {
        for( int idx : slots )
          if( try_add( id, idx ) ) //
            return true;
        return false;
      },
      [&]( Commodity const& c ) {
        auto old_slots = slots_;
        auto commodity = c; // make copy.
        CHECK( commodity.quantity > 0 );
        for( int idx : slots ) {
          if( commodity.quantity == 0 ) break;
          switch( auto& v = slots_[idx]; v.to_enum() ) {
            case CargoSlot::e::empty: {
              auto quantity_to_add =
                  std::min( commodity.quantity,
                            k_max_commodity_cargo_per_slot );
              CHECK( quantity_to_add > 0 );
              commodity.quantity -= quantity_to_add;
              CHECK( try_add( Commodity{
                                  /*type=*/commodity.type,
                                  /*quantity=*/quantity_to_add },
                              idx ),
                     "failed to add commodity of type {} and "
                     "quantity {} to slot {}",
                     commodity.type, quantity_to_add, idx )
              break;
            }
            case CargoSlot::e::overflow: break;
            case CargoSlot::e::cargo: {
              auto& cargo = v.get<CargoSlot::cargo>();
              if( auto* comm_in_slot = get_if<Commodity>(
                      &( cargo.contents ) ) ) {
                if( comm_in_slot->type == commodity.type ) {
                  auto quantity_to_add =
                      std::min( commodity.quantity,
                                k_max_commodity_cargo_per_slot -
                                    comm_in_slot->quantity );
                  commodity.quantity -= quantity_to_add;
                  if( quantity_to_add > 0 ) {
                    CHECK(
                        try_add(
                            Commodity{
                                /*type=*/commodity.type,
                                /*quantity=*/quantity_to_add },
                            idx ),
                        "failed to add commodity of type {} and "
                        "quantity {} to slot {}",
                        commodity.type, quantity_to_add, idx )
                  }
                }
              }
              break;
            }
          }
        }
        if( commodity.quantity == 0 )
          return true;
        else {
          // Couldn't make it work, so restore state.
          slots_ = old_slots;
          return false;
        }
      } );
}

bool CargoHold::try_add( Cargo const& cargo, int slot ) {
  if( auto* id = get_if<UnitId>( &cargo ) ) {
    // Make sure that the unit is not already in this cargo.
    auto units = items_of_type<UnitId>();
    auto this_unit_in_cargo =
        util::count_if( units, LC( _ == *id ) );
    CHECK( this_unit_in_cargo == 0 );
  }
  if( !fits( cargo, slot ) ) return false;
  // From here on we assume it is totally safe in every way to
  // blindly add this cargo into the given slot(s).
  auto was_added = overload_visit(
      cargo,
      [&]( UnitId id ) {
        auto maybe_occupied =
            unit_from_id( id ).desc().cargo_slots_occupies;
        if( !maybe_occupied ) return false;
        auto occupied = *maybe_occupied;
        slots_[slot]  = CargoSlot::cargo{ /*contents=*/cargo };
        // Now handle overflow.
        while( slot++, occupied-- > 1 )
          slots_[slot] = CargoSlot::overflow{};
        return true;
      },
      [&]( Commodity const& c ) {
        if( holds<CargoSlot::empty>( slots_[slot] ) )
          slots_[slot] = CargoSlot::cargo{ /*contents=*/cargo };
        else {
          ASSIGN_CHECK_V( cargo, slots_[slot],
                          CargoSlot::cargo );
          ASSIGN_CHECK_V( comm, cargo.contents, Commodity );
          CHECK( comm.type == c.type );
          comm.quantity += c.quantity;
        }
        return true;
      } );
  check_invariants_or_abort();
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
  check_invariants_or_abort();
}

void CargoHold::clear() {
  for( auto& slot : slots_ ) //
    slot = CargoSlot::empty{};
  check_invariants_or_abort();
}

maybe<Cargo const&> CargoHold::cargo_starting_at_slot(
    int idx ) const {
  CHECK( idx >= 0 && idx < slots_total() );
  return slots_[idx]              //
      .get_if<CargoSlot::cargo>() //
      .member( &CargoSlot::cargo::contents );
}

maybe<pair<Cargo const&, int>> CargoHold::cargo_covering_slot(
    int idx ) const {
  CHECK( idx >= 0 && idx < slots_total() );
  if( slots_[idx].holds<CargoSlot::empty>() ) return nothing;
  do {
    maybe<Cargo const&> ref =
        slots_[idx]
            .get_if<CargoSlot::cargo>() //
            .member( &CargoSlot::cargo::contents );
    // Need to explicitly specify the types for this pair other-
    // wise it will infer a Cargo by value, then implicitely con-
    // vert to a reference (for our return type) and thus will
    // end up returning reference to a temporary.
    if( ref ) return pair<Cargo const&, int>{ *ref, idx };
  } while( --idx >= 0 );
  return nothing;
}

} // namespace rn
