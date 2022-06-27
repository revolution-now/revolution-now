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
#include "logger.hpp"
#include "macros.hpp"
#include "util.hpp"
#include "variant.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"

// C++ standard library
#include <type_traits>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

constexpr int const k_max_commodity_cargo_per_slot = 100;

} // namespace

CargoHold::CargoHold( int num_slots )
  : o_( wrapped::CargoHold{
        .slots = vector<CargoSlot_t>( num_slots ) } ) {}

base::valid_or<string> wrapped::CargoHold::validate() const {
  int slots_total = slots.size();
  // 1. First slot is not an `overflow`.
  if( slots.size() > 0 )
    REFL_VALIDATE( !holds<CargoSlot::overflow>( slots[0] ), "" );
  // 2. There are no `overflow`s following `empty`s.
  for( int i = 0; i < slots_total - 1; ++i )
    if( holds<CargoSlot::empty>( slots[i] ) )
      REFL_VALIDATE( !holds<CargoSlot::overflow>( slots[i + 1] ),
                     "" );
  // 3. There are no `overflow`s following `commodity`s.
  for( int i = 0; i < slots_total - 1; ++i ) {
    if( auto* cargo = get_if<CargoSlot::cargo>( &slots[i] ) )
      if( holds<Cargo::commodity>( cargo->contents ) )
        REFL_VALIDATE(
            !holds<CargoSlot::overflow>( slots[i + 1] ), "" );
  }
  // 4. Commodities don't exceed max quantity and are not zero
  // quantity.
  for( auto const& slot : slots ) {
    if( auto* cargo = get_if<CargoSlot::cargo>( &slot ) ) {
      if( auto* commodity = get_if<Cargo::commodity>(
              &( cargo->contents ) ) ) {
        REFL_VALIDATE( commodity->obj.quantity <=
                           k_max_commodity_cargo_per_slot,
                       "" );
        REFL_VALIDATE( commodity->obj.quantity > 0, "" );
      }
    }
  }
  return valid;
}

void CargoHold::validate_or_die(
    UnitsState const& units_state ) const {
  CHECK_HAS_VALUE( validate( units_state ) );
}

valid_or<generic_err> CargoHold::validate(
    UnitsState const& units_state ) const {
  // First validate the reflected state. This will do the valida-
  // tion that can be done without needing access to any game
  // state outside of this cargo object.
  base::valid_or<string> res = o_.validate();
  if( !res ) return GENERIC_ERROR( "{}", res.error() );

  // Now do some validation steps that require access to all
  // units.

  // 1. Units with overflow are properly followed by `overflow`.
  for( int i = 0; i < slots_total(); ++i ) {
    auto const& slot = o_.slots[i];
    if( auto* cargo = get_if<CargoSlot::cargo>( &slot ) ) {
      if( auto* u =
              get_if<Cargo::unit>( &( cargo->contents ) ) ) {
        auto const& unit = units_state.unit_for( u->id );
        auto        occupies =
            unit.desc().cargo_slots_occupies.value_or( 0 );
        TRUE_OR_RETURN_GENERIC_ERR( occupies > 0 );
        // Check for overflow slots.
        while( occupies > 1 ) {
          --occupies;
          ++i;
          TRUE_OR_RETURN_GENERIC_ERR( i < slots_total() );
          TRUE_OR_RETURN_GENERIC_ERR(
              holds<CargoSlot::overflow>( o_.slots[i] ) );
        }
      }
    }
  }

  // 2. Slots occupied matches real contents.
  int occupied = 0;
  for( int i = 0; i < slots_total(); ++i ) {
    auto const& slot = o_.slots[i];
    switch( slot.to_enum() ) {
      case CargoSlot::e::empty: //
        break;
      case CargoSlot::e::overflow: //
        break;
      case CargoSlot::e::cargo: {
        auto& cargo = slot.get<CargoSlot::cargo>();
        overload_visit(
            cargo.contents,
            [&]( Cargo::unit u ) {
              occupied +=
                  units_state.unit_for( u.id )
                      .desc()
                      .cargo_slots_occupies.value_or( 0 );
            },
            [&]( Cargo::commodity const& ) { occupied++; } );
        break;
      }
    }
  }
  TRUE_OR_RETURN_GENERIC_ERR( occupied == slots_occupied() );
  return base::valid;
}

int CargoHold::max_commodity_per_cargo_slot() const {
  return k_max_commodity_cargo_per_slot;
}

int CargoHold::slots_occupied() const {
  return slots_total() - slots_remaining();
}

int CargoHold::slots_remaining() const {
  return util::count_if( o_.slots,
                         L( holds<CargoSlot::empty>( _ ) ) );
}

int CargoHold::slots_total() const { return o_.slots.size(); }

int CargoHold::count_items() const {
  return util::count_if( o_.slots,
                         L( holds<CargoSlot::cargo>( _ ) ) );
}

maybe<CargoSlot_t const&> CargoHold::at( int slot ) const {
  if( slot < 0 || slot >= slots_total() ) return nothing;
  return ( *this )[slot];
}

CargoSlot_t const& CargoHold::operator[]( int idx ) const {
  CHECK( idx >= 0 && idx < int( o_.slots.size() ) );
  return o_.slots[idx];
}

CargoSlot_t& CargoHold::operator[]( int idx ) {
  CHECK( idx >= 0 && idx < int( o_.slots.size() ) );
  return o_.slots[idx];
}

maybe<int> CargoHold::find_unit( UnitId id ) const {
  for( size_t idx = 0; idx < o_.slots.size(); ++idx )
    if( slot_holds_cargo_type<Cargo::unit>( idx ) ==
        Cargo::unit{ id } ) //
      return idx;
  return nothing;
}

// Returns all units in the cargo.
vector<UnitId> CargoHold::units() const {
  vector<UnitId> res;
  for( auto unit : items_of_type<Cargo::unit>() )
    res.push_back( unit.id );
  return res;
}

// Returns all commodities (and slot indices) in the cargo un-
// less a specific type is specified in which case it will be
// limited to those.
vector<pair<Commodity, int>> CargoHold::commodities(
    maybe<e_commodity> type ) const {
  vector<pair<Commodity, int>> res;

  for( auto const& [idx, slot] :
       rl::all( o_.slots ).enumerate() ) {
    if( auto* cargo = get_if<CargoSlot::cargo>( &slot ) )
      if( auto* commodity =
              get_if<Cargo::commodity>( &( cargo->contents ) ) )
        if( !type || ( commodity->obj.type == *type ) )
          res.emplace_back( commodity->obj, idx );
  }
  return res;
}

void CargoHold::compactify( UnitsState const& units_state ) {
  auto unit_ids    = units();
  auto comms_pairs = commodities();
  auto comms       = rl::all( comms_pairs ).keys().to_vector();
  // First sort by ID, then do a stable sort on slot occupancy to
  // get "deterministic" results.
  util::sort_by_key( unit_ids, []( auto id ) { return id; } );
  // Negative to do reverse sort.
  util::stable_sort_by_key(
      unit_ids, LC( -units_state.unit_for( _ )
                         .desc()
                         .cargo_slots_occupies.value_or( 0 ) ) );
  util::sort_by_key( comms, L( _.type ) );
  clear();
  validate_or_die( units_state );
  for( UnitId id : unit_ids )
    CHECK( try_add_somewhere( units_state, Cargo::unit{ id } ) );
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
      CHECK( try_add_somewhere( units_state,
                                Cargo::commodity{ comm } ) );
  }
  validate_or_die( units_state );
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
            cargo.contents, //
            []( Cargo::unit ) { return 0; },
            [&]( Cargo::commodity const& c ) {
              return ( c.obj.type == type )
                         ? ( k_max_commodity_cargo_per_slot -
                             c.obj.quantity )
                         : 0;
            } );
      }
    }
  };
  return rl::all( o_.slots ).map( one_slot ).accumulate();
}

bool CargoHold::fits( UnitsState const& units_state,
                      Cargo_t const& cargo, int slot ) const {
  CHECK( slot >= 0 && slot < int( o_.slots.size() ) );
  return overload_visit(
      cargo,
      [&]( Cargo::unit u ) {
        auto maybe_occupied = units_state.unit_for( u.id )
                                  .desc()
                                  .cargo_slots_occupies;
        if( !maybe_occupied )
          // Unit cannot be held as cargo.
          return false;
        auto occupied = *maybe_occupied;
        // Check that all needed slots are `empty`.
        for( int i = slot; i < slot + occupied; ++i ) {
          if( i >= slots_total() )
            // Not enough slots left.
            return false;
          if( !holds<CargoSlot::empty>( o_.slots[i] ) )
            // Needed slots are not empty.
            return false;
        }
        return true;
      },
      [&]( Cargo::commodity const& c ) {
        auto const& proposed = c.obj;
        if( proposed.quantity > k_max_commodity_cargo_per_slot )
          return false;
        if( proposed.quantity == 0 ) //
          return false;
        switch( auto& v = o_.slots[slot]; v.to_enum() ) {
          case CargoSlot::e::overflow: {
            return false;
          }
          case CargoSlot::e::empty: {
            return true;
          }
          case CargoSlot::e::cargo: {
            auto& cargo = v.get<CargoSlot::cargo>();
            return overload_visit(
                cargo.contents,
                []( Cargo::unit ) { return false; },
                [&]( Cargo::commodity const& c ) {
                  if( proposed.type != c.obj.type ) //
                    return false;
                  return ( c.obj.quantity + proposed.quantity <=
                           k_max_commodity_cargo_per_slot );
                } );
            break;
          }
        }
      } );
}

ND bool CargoHold::fits_with_item_removed(
    UnitsState const& units_state, Cargo_t const& cargo,
    CargoSlotIndex remove_slot,
    CargoSlotIndex insert_slot ) const {
  CargoHold new_hold = *this;
  new_hold.remove( remove_slot );
  return new_hold.fits( units_state, cargo, insert_slot );
}

ND bool CargoHold::fits_somewhere_with_item_removed(
    UnitsState const& units_state, Cargo_t const& cargo,
    int remove_slot, int starting_slot ) const {
  CargoHold new_hold = *this;
  new_hold.remove( remove_slot );
  return new_hold.fits_somewhere( units_state, cargo,
                                  starting_slot );
}

bool CargoHold::fits_somewhere( UnitsState const& units_state,
                                Cargo_t const&    cargo,
                                int starting_slot ) const {
  CargoHold new_hold = *this;
  // Do this so that this tmp cargo hold does not get destroyed
  // with stuff in it, which currently triggers a warning to be
  // logged to the console and slows things down.
  SCOPE_EXIT( new_hold.o_.slots.clear() );
  return new_hold.try_add_somewhere( units_state, cargo,
                                     starting_slot );
}

bool CargoHold::try_add_somewhere( UnitsState const& units_state,
                                   Cargo_t const&    cargo,
                                   int starting_from ) {
  if( slots_total() == 0 ) return false;
  CHECK( starting_from >= 0 && starting_from < slots_total() );
  auto slots = rl::ints( 0, slots_total() )
                   .cycle()
                   .drop( starting_from )
                   .take( slots_total() );
  return overload_visit(
      cargo,
      [&]( Cargo::unit u ) {
        for( int idx : slots )
          if( try_add( units_state, u, idx ) ) //
            return true;
        return false;
      },
      [&]( Cargo::commodity const& c ) {
        auto old_slots = o_.slots;
        auto commodity = c.obj; // make copy.
        CHECK( commodity.quantity > 0 );
        for( int idx : slots ) {
          if( commodity.quantity == 0 ) break;
          switch( auto& v = o_.slots[idx]; v.to_enum() ) {
            case CargoSlot::e::empty: {
              auto quantity_to_add =
                  std::min( commodity.quantity,
                            k_max_commodity_cargo_per_slot );
              CHECK( quantity_to_add > 0 );
              commodity.quantity -= quantity_to_add;
              CHECK(
                  try_add( units_state,
                           Cargo::commodity{ Commodity{
                               /*type=*/commodity.type,
                               /*quantity=*/quantity_to_add } },
                           idx ),
                  "failed to add commodity of type {} and "
                  "quantity {} to slot {}",
                  commodity.type, quantity_to_add, idx )
              break;
            }
            case CargoSlot::e::overflow: break;
            case CargoSlot::e::cargo: {
              auto& cargo = v.get<CargoSlot::cargo>();
              if( auto* comm_in_slot = get_if<Cargo::commodity>(
                      &( cargo.contents ) ) ) {
                if( comm_in_slot->obj.type == commodity.type ) {
                  auto quantity_to_add =
                      std::min( commodity.quantity,
                                k_max_commodity_cargo_per_slot -
                                    comm_in_slot->obj.quantity );
                  commodity.quantity -= quantity_to_add;
                  if( quantity_to_add > 0 ) {
                    CHECK(
                        try_add(
                            units_state,
                            Cargo::commodity{ Commodity{
                                /*type=*/commodity.type,
                                /*quantity=*/quantity_to_add } },
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
          o_.slots = old_slots;
          return false;
        }
      } );
}

bool CargoHold::try_add( UnitsState const& units_state,
                         Cargo_t const& cargo, int slot ) {
  if( auto* unit = get_if<Cargo::unit>( &cargo ) ) {
    UnitId id = unit->id;
    // Make sure that the unit is not already in this cargo.
    auto units = items_of_type<Cargo::unit>();
    auto this_unit_in_cargo =
        util::count_if( units, LC( _.id == id ) );
    CHECK( this_unit_in_cargo == 0 );
  }
  if( !fits( units_state, cargo, slot ) ) return false;
  // From here on we assume it is totally safe in every way to
  // blindly add this cargo into the given slot(s).
  auto was_added = overload_visit(
      cargo,
      [&]( Cargo::unit u ) {
        UnitId id             = u.id;
        auto   maybe_occupied = units_state.unit_for( id )
                                  .desc()
                                  .cargo_slots_occupies;
        if( !maybe_occupied ) return false;
        auto occupied  = *maybe_occupied;
        o_.slots[slot] = CargoSlot::cargo{ /*contents=*/cargo };
        // Now handle overflow.
        while( slot++, occupied-- > 1 )
          o_.slots[slot] = CargoSlot::overflow{};
        return true;
      },
      [&]( Cargo::commodity const& c ) {
        if( holds<CargoSlot::empty>( o_.slots[slot] ) )
          o_.slots[slot] =
              CargoSlot::cargo{ /*contents=*/cargo };
        else {
          ASSIGN_CHECK_V( cargo, o_.slots[slot],
                          CargoSlot::cargo );
          ASSIGN_CHECK_V( comm, cargo.contents,
                          Cargo::commodity );
          CHECK( comm.obj.type == c.obj.type );
          comm.obj.quantity += c.obj.quantity;
        }
        return true;
      } );
  validate_or_die( units_state );
  return was_added;
}

void CargoHold::remove( int slot ) {
  CHECK( slot >= 0 && slot < int( o_.slots.size() ) );
  CHECK( holds<CargoSlot::cargo>( o_.slots[slot] ) );
  o_.slots[slot] = CargoSlot::empty{};
  slot++;
  while( slot < int( o_.slots.size() ) &&
         holds<CargoSlot::overflow>( o_.slots[slot] ) ) {
    o_.slots[slot] = CargoSlot::empty{};
    ++slot;
  }
}

void CargoHold::clear() {
  for( auto& slot : o_.slots ) //
    slot = CargoSlot::empty{};
}

maybe<Cargo_t const&> CargoHold::cargo_starting_at_slot(
    int idx ) const {
  CHECK( idx >= 0 && idx < slots_total() );
  return o_
      .slots[idx]                 //
      .get_if<CargoSlot::cargo>() //
      .member( &CargoSlot::cargo::contents );
}

maybe<pair<Cargo_t const&, int>> CargoHold::cargo_covering_slot(
    int idx ) const {
  CHECK( idx >= 0 && idx < slots_total() );
  if( o_.slots[idx].holds<CargoSlot::empty>() ) return nothing;
  do {
    maybe<Cargo_t const&> ref =
        o_.slots[idx]
            .get_if<CargoSlot::cargo>() //
            .member( &CargoSlot::cargo::contents );
    // Need to explicitly specify the types for this pair other-
    // wise it will infer a Cargo by value, then implicitely con-
    // vert to a reference (for our return type) and thus will
    // end up returning reference to a temporary.
    if( ref ) return pair<Cargo_t const&, int>{ *ref, idx };
  } while( --idx >= 0 );
  return nothing;
}

} // namespace rn
