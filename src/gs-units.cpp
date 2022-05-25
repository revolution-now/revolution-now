/****************************************************************
**gs-units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Unit-related save-game state.
*
*****************************************************************/
#include "gs-units.hpp"

// Revolution Now
#include "cstate.hpp"
#include "logger.hpp"
#include "ustate.hpp"
#include "variant.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

constexpr int kFirstUnitId = 1;

} // namespace

/****************************************************************
** wrapped::UnitsState
*****************************************************************/
valid_or<string> wrapped::UnitsState::validate() const {
  // Check for no free units.
  for( auto const& [id, unit_state] : units ) {
    UnitOwnership_t const& st = unit_state.ownership;
    REFL_VALIDATE( !holds<UnitOwnership::free>( st ),
                   "unit {} is in the `free` state.", id );
  }

  // Check old_world states.
  for( auto const& [id, unit_state] : units ) {
    UnitOwnership_t const& st = unit_state.ownership;
    if_get( st, UnitOwnership::old_world, val ) {
      REFL_VALIDATE(
          check_old_world_state_invariants( val.st ) );
    }
  }

  // Validate all unit cargos. We can only do this now after
  // all units have been loaded.
  for( auto const& [id, unit_state] : units )
    // FIXME: this can lead to a crash when a save-game is loaded
    // while an existing game is in progress where a unit was
    // lost due to it having been cargo of a sunk ship, then the
    // save-game being loaded has a unit with that same ID that
    // is also in the ship cargo. This crash happens because the
    // cargo, as it performs its validation, will query for the
    // unit state, but it will end up querying the current unit
    // state and not the one we are validating here. The proper
    // fix for this requires passing the game state as a para-
    // meter to whoever needs it as opposed to code getting it
    // from globals.
    REFL_VALIDATE( unit_state.unit.cargo().validate() );
  return base::valid;
}

/****************************************************************
** UnitsState
*****************************************************************/
valid_or<std::string> UnitsState::validate() const {
  HAS_VALUE_OR_RET( o_.validate() );
  // No further validation on derived state for now.
  return base::valid;
}

void UnitsState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

UnitsState::UnitsState( wrapped::UnitsState&& o )
  : o_( std::move( o ) ) {
  // Populate units_from_coords_.
  for( auto const& [id, unit_state] : o_.units ) {
    UnitOwnership_t const& st = unit_state.ownership;
    if_get( st, UnitOwnership::world, val ) {
      units_from_coords_[val.coord].insert( id );
    }
  }

  // Populate worker_units_from_colony_.
  for( auto const& [id, unit_state] : o_.units ) {
    UnitOwnership_t const& st = unit_state.ownership;
    if_get( st, UnitOwnership::colony, val ) {
      worker_units_from_colony_[val.id].insert( id );
    }
  }
}

UnitsState::UnitsState()
  : UnitsState( wrapped::UnitsState{
        .next_unit_id = kFirstUnitId, .units = {} } ) {
  validate_or_die();
}

unordered_map<UnitId, UnitState> const& UnitsState::all() const {
  return o_.units;
}

UnitState const& UnitsState::state_of( UnitId id ) const {
  CHECK( !deleted_.contains( id ),
         "unit with ID {} existed but was deleted.", id );
  UNWRAP_CHECK_MSG( unit_state, base::lookup( o_.units, id ),
                    "unit {} does not exist.", id );
  return unit_state;
}

UnitState& UnitsState::state_of( UnitId id ) {
  CHECK( !deleted_.contains( id ),
         "unit with ID {} existed but was deleted.", id );
  UNWRAP_CHECK_MSG( unit_state, base::lookup( o_.units, id ),
                    "unit {} does not exist.", id );
  return unit_state;
}

UnitOwnership_t const& UnitsState::ownership_of(
    UnitId id ) const {
  return state_of( id ).ownership;
}

UnitOwnership_t& UnitsState::ownership_of( UnitId id ) {
  return state_of( id ).ownership;
}

Unit const& UnitsState::unit_for( UnitId id ) const {
  return state_of( id ).unit;
}

Unit& UnitsState::unit_for( UnitId id ) {
  return state_of( id ).unit;
}

maybe<Coord> UnitsState::maybe_coord_for( UnitId id ) const {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::world:
      return o.get<UnitOwnership::world>().coord;
    case UnitOwnership::e::free:
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::old_world:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

Coord UnitsState::coord_for( UnitId id ) const {
  UNWRAP_CHECK_MSG( coord, maybe_coord_for( id ),
                    "unit is not on map." );
  return coord;
}

maybe<UnitId> UnitsState::maybe_holder_of( UnitId id ) const {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::cargo:
      return o.get<UnitOwnership::cargo>().holder;
    case UnitOwnership::e::world:
    case UnitOwnership::e::free:
    case UnitOwnership::e::old_world:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

UnitId UnitsState::holder_of( UnitId id ) const {
  UNWRAP_CHECK_MSG( holder, maybe_holder_of( id ),
                    "unit is not being held as cargo." );
  return holder;
}

maybe<UnitOldWorldViewState_t&>
UnitsState::maybe_old_world_view_state_of( UnitId id ) {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::old_world:
      return o.get<UnitOwnership::old_world>().st;
    case UnitOwnership::e::world:
    case UnitOwnership::e::free:
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

UnitOldWorldViewState_t& UnitsState::old_world_view_state_of(
    UnitId id ) {
  UNWRAP_CHECK_MSG( st, maybe_old_world_view_state_of( id ),
                    "unit is not in the old world state." );
  return st;
}

void UnitsState::disown_unit( UnitId id ) {
  auto& ownership = ownership_of( id );
  switch( auto& v = ownership; v.to_enum() ) {
    case UnitOwnership::e::free: //
      break;
    case UnitOwnership::e::world: {
      auto& [coord] = v.get<UnitOwnership::world>();
      UNWRAP_CHECK( set_it,
                    base::find( units_from_coords_, coord ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() ) units_from_coords_.erase( set_it );
      break;
    }
    case UnitOwnership::e::cargo: {
      auto& holder_unit = unit_for( holder_of( id ) );
      UNWRAP_CHECK( slot_idx,
                    holder_unit.cargo().find_unit( id ) );
      holder_unit.cargo().remove( slot_idx );
      break;
    }
    case UnitOwnership::e::old_world: {
      break;
    }
    case UnitOwnership::e::colony: {
      auto& val    = v.get<UnitOwnership::colony>();
      auto  col_id = val.id;
      UNWRAP_CHECK(
          set_it,
          base::find( worker_units_from_colony_, col_id ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() )
        worker_units_from_colony_.erase( set_it );
      colony_from_id( col_id ).remove_unit( id );
      break;
    }
  };
  ownership = UnitOwnership::free{};
}

void UnitsState::change_to_map( UnitId id, Coord target ) {
  disown_unit( id );
  units_from_coords_[target].insert( id );
  ownership_of( id ) = UnitOwnership::world{ /*coord=*/target };
}

void UnitsState::change_to_cargo_somewhere( UnitId new_holder,
                                            UnitId held,
                                            int starting_slot ) {
  auto& cargo = unit_for( new_holder ).cargo();
  for( int i = starting_slot;
       i < starting_slot + cargo.slots_total(); ++i ) {
    int modded = i % cargo.slots_total();
    if( cargo.fits( Cargo::unit{ held }, modded ) ) {
      change_to_cargo( new_holder, held, modded );
      return;
    }
  }
  FATAL( "{} cannot be placed in {}'s cargo: {}",
         debug_string( held ), debug_string( new_holder ),
         cargo );
}

void UnitsState::change_to_cargo( UnitId new_holder, UnitId held,
                                  int slot ) {
  // Make sure that we're not adding the unit to its own cargo.
  // Should never happen theoretically, but...
  CHECK( new_holder != held );
  // Check that the proposed `held` unit cannot itself hold
  // cargo, because it is a game rule that cargo-holding units
  // cannot be cargo of other units.
  CHECK( unit_for( held ).desc().cargo_slots == 0 );
  // Check that the proposed `held` unit can occupy cargo.
  CHECK(
      unit_for( held ).desc().cargo_slots_occupies.has_value() );
  auto& cargo_hold = unit_for( new_holder ).cargo();
  // We're clear (at least on our end).
  disown_unit( held );
  // Check that there are enough open slots. Note we do this
  // after disowning the unit just in case we are moving the unit
  // into a cargo slot that it already occupies or moving a large
  // unit (i.e., one occupying multiple slots) to another slot in
  // the same cargo where it will not fit unless it is first re-
  // moved from its current slot.
  CHECK( cargo_hold.fits( Cargo::unit{ held }, slot ) );
  CHECK( cargo_hold.try_add( Cargo::unit{ held }, slot ) );
  unit_for( held ).sentry();
  // Set new ownership
  ownership_of( held ) =
      UnitOwnership::cargo{ /*holder=*/new_holder };
}

void UnitsState::change_to_old_world_view(
    UnitId id, UnitOldWorldViewState_t info ) {
  CHECK_HAS_VALUE( check_old_world_state_invariants( info ) );
  UnitOwnership_t& ownership = ownership_of( id );
  if( !ownership.holds<UnitOwnership::old_world>() )
    disown_unit( id );
  ownership = UnitOwnership::old_world{ /*st=*/info };
}

void UnitsState::change_to_colony( UnitId id, ColonyId col_id,
                                   ColonyJob_t const& job ) {
  CHECK( unit_for( id ).nation() ==
         colony_from_id( col_id ).nation() );
  disown_unit( id );
  worker_units_from_colony_[col_id].insert( id );
  ownership_of( id ) = UnitOwnership::colony{ col_id };
  colony_from_id( col_id ).add_unit( id, job );
}

UnitId UnitsState::add_unit( Unit&& unit ) {
  CHECK( unit.id() == UnitId{ 0 },
         "unit ID must be zero when creating unit." );
  UnitId id  = next_unit_id();
  unit.o_.id = id;
  DCHECK( !o_.units.contains( id ) );
  DCHECK( !deleted_.contains( id ) );
  o_.units[id] = UnitState{ .unit      = std::move( unit ),
                            .ownership = UnitOwnership::free{} };
  return id;
}

void UnitsState::destroy_unit( UnitId id ) {
  CHECK( o_.units.contains( id ) );
  CHECK( !deleted_.contains( id ) );
  auto& unit = unit_for( id );
  // Recursively destroy any units in the cargo. We must get the
  // list of units to destroy first because we don't want to
  // destroy a cargo unit while iterating over the cargo.
  vector<UnitId> cargo_units_to_destroy;
  for( auto const& cargo_unit :
       unit.cargo().items_of_type<Cargo::unit>() ) {
    lg.debug(
        "{} being destroyed as a consequence of {} being "
        "destroyed",
        debug_string( unit_for( cargo_unit.id ) ),
        debug_string( unit_for( id ) ) );
    cargo_units_to_destroy.push_back( cargo_unit.id );
  }
  for( UnitId to_destroy : cargo_units_to_destroy )
    destroy_unit( to_destroy );
  disown_unit( id );

  o_.units.erase( id );
  deleted_.insert( id );
}

bool UnitsState::exists( UnitId id ) const {
  // Note that we don't consult the deleted units cache here
  // since that is only transient state. It is more robust to
  // check the next_unit_id.
  CHECK( id._ < o_.next_unit_id, "unit id {} never existed.",
         id );
  return o_.units.contains( id );
}

UnitId UnitsState::next_unit_id() {
  return UnitId{ o_.next_unit_id++ };
}

UnitId UnitsState::last_unit_id() const {
  CHECK( o_.next_unit_id > 0, "no units yet created." );
  return UnitId{ o_.next_unit_id };
}

unordered_set<UnitId> const& UnitsState::from_coord(
    Coord const& coord ) const {
  static unordered_set<UnitId> const empty = {};
  // CHECK( square_exists( c ) );
  return base::lookup( units_from_coords_, coord )
      .value_or( empty );
}

unordered_set<UnitId> const& UnitsState::from_colony(
    ColonyId id ) const {
  // FIXME: consider reinstating this check once we can pass in
  // global state as a parameter. In this case, we should pass in
  // the colony state. As it is now, we can't call it because it
  // breaks during load-game validation, since it is called by
  // said validation but it queries the current global state (and
  // not the one being validated).
  // CHECK( colony_exists( id ), "colony {} does not exist.", id
  // );
  UNWRAP_CHECK( units,
                base::lookup( worker_units_from_colony_, id ) );
  return units;
}

} // namespace rn
