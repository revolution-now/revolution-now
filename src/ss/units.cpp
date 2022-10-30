/****************************************************************
**units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Unit-related save-game state.
*
*****************************************************************/
#include "units.hpp"

// Revolution Now
#include "logger.hpp"
#include "variant.hpp"

// config
#include "config/unit-type.rds.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

constexpr GenericUnitId kFirstUnitId{ 1 };

// FIXME: we should have a generic way to do this.
valid_or<generic_err> check_harbor_state_invariants(
    UnitHarborViewState const& info ) {
  valid_or<string> res =
      std::visit( []( auto const& o ) { return o.validate(); },
                  info.port_status );
  if( !res ) return GENERIC_ERROR( "{}", res.error() );
  return base::valid;
}

} // namespace

/****************************************************************
** wrapped::UnitsState
*****************************************************************/
valid_or<string> wrapped::UnitsState::validate() const {
  // Check for no free units.
  for( auto const& [id, unit_state] : units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        UnitOwnership_t const& st = o.state.ownership;
        REFL_VALIDATE( !holds<UnitOwnership::free>( st ),
                       "unit {} is in the `free` state.", id );
        break;
      }
      case UnitState::e::native: {
        auto& o = unit_state.get<UnitState::native>();
        NativeUnitOwnership_t const& st = o.state.ownership;
        REFL_VALIDATE( !holds<NativeUnitOwnership::free>( st ),
                       "unit {} is in the `free` state.", id );
        break;
      }
    }
  }
  return base::valid;
}

/****************************************************************
** UnitsState
*****************************************************************/
valid_or<std::string> UnitsState::validate() const {
  HAS_VALUE_OR_RET( o_.validate() );

  // Validate all unit cargos. We can only do this now after
  // all units have been loaded.
  for( auto const& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        REFL_VALIDATE( o.state.unit.cargo().validate( *this ) );
        break;
      }
      case UnitState::e::native: {
        break;
      }
    }
  }

  return base::valid;
}

void UnitsState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

UnitsState::UnitsState( wrapped::UnitsState&& o )
  : o_( std::move( o ) ) {
  // Populate units_from_coords_.
  for( auto const& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        UnitOwnership_t const& st = o.state.ownership;
        if_get( st, UnitOwnership::world, val ) {
          units_from_coords_[val.coord].insert( id );
        }
        break;
      }
      case UnitState::e::native: {
        auto& o = unit_state.get<UnitState::native>();
        NativeUnitOwnership_t const& st = o.state.ownership;
        if_get( st, NativeUnitOwnership::world, val ) {
          units_from_coords_[val.coord].insert( id );
        }
        break;
      }
    }
  }

  // Populate worker_units_from_colony_.
  for( auto const& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        UnitOwnership_t const& st = o.state.ownership;
        if_get( st, UnitOwnership::colony, val ) {
          worker_units_from_colony_[val.id].insert(
              UnitId{ to_underlying( id ) } );
        }
        break;
      }
      case UnitState::e::native: break;
    }
  }

  // Populate unit kinds.
  for( auto& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        euro_units_[UnitId{ to_underlying( id ) }] = &o.state;
        break;
      }
      case UnitState::e::native: {
        auto& o = unit_state.get<UnitState::native>();
        native_units_[NativeUnitId{ to_underlying( id ) }] =
            &o.state;
        break;
      }
    }
  }
}

UnitsState::UnitsState()
  : UnitsState( wrapped::UnitsState{
        .next_unit_id = kFirstUnitId, .units = {} } ) {
  validate_or_die();
}

bool UnitsState::operator==( UnitsState const& rhs ) const {
  return o_ == rhs.o_;
}

e_unit_kind UnitsState::unit_kind(
    GenericUnitId generic_id ) const {
  CHECK( exists( generic_id ), "unit {} does not exist.",
         generic_id );
  int const id = to_underlying( generic_id );
  if( euro_units_.contains( UnitId{ id } ) )
    return e_unit_kind::euro;
  if( native_units_.contains( NativeUnitId{ id } ) )
    return e_unit_kind::native;
  FATAL(
      "unit {} exists but it was not found in either the list "
      "of european units or native units.",
      generic_id );
}

UnitId UnitsState::check_euro_unit( GenericUnitId id ) const {
  CHECK( unit_kind( id ) == e_unit_kind::euro );
  return UnitId{ to_underlying( id ) };
}

NativeUnitId UnitsState::check_native_unit(
    GenericUnitId id ) const {
  CHECK( unit_kind( id ) == e_unit_kind::native );
  return NativeUnitId{ to_underlying( id ) };
}

unordered_map<GenericUnitId, UnitState_t> const&
UnitsState::all() const {
  return o_.units;
}

unordered_map<UnitId, EuroUnitState const*> const&
UnitsState::euro_all() const {
  return euro_units_;
}

unordered_map<NativeUnitId, NativeUnitState const*> const&
UnitsState::native_all() const {
  return native_units_;
}

UnitState_t const& UnitsState::state_of(
    GenericUnitId id ) const {
  CHECK( !deleted_.contains( id ),
         "unit with ID {} existed but was deleted.", id );
  UNWRAP_CHECK_MSG( unit_state, base::lookup( o_.units, id ),
                    "unit {} does not exist.", id );
  return unit_state;
}

UnitState_t& UnitsState::state_of( GenericUnitId id ) {
  CHECK( !deleted_.contains( id ),
         "unit with ID {} existed but was deleted.", id );
  UNWRAP_CHECK_MSG( unit_state, base::lookup( o_.units, id ),
                    "unit {} does not exist.", id );
  return unit_state;
}

EuroUnitState const& UnitsState::state_of( UnitId id ) const {
  UNWRAP_CHECK_MSG( unit_state, base::lookup( euro_units_, id ),
                    "unit {} does not exist.", id );
  return *unit_state;
}

NativeUnitState const& UnitsState::state_of(
    NativeUnitId id ) const {
  UNWRAP_CHECK_MSG( unit_state,
                    base::lookup( native_units_, id ),
                    "unit {} does not exist.", id );
  return *unit_state;
}

EuroUnitState& UnitsState::state_of( UnitId id ) {
  UnitState_t& generic_state =
      state_of( GenericUnitId{ to_underlying( id ) } );
  UNWRAP_CHECK( euro_unit_state,
                generic_state.get_if<UnitState::euro>() );
  return euro_unit_state.state;
}

NativeUnitState& UnitsState::state_of( NativeUnitId id ) {
  UnitState_t& generic_state =
      state_of( GenericUnitId{ to_underlying( id ) } );
  UNWRAP_CHECK( native_unit_state,
                generic_state.get_if<UnitState::native>() );
  return native_unit_state.state;
}

UnitOwnership_t const& UnitsState::ownership_of(
    UnitId id ) const {
  return state_of( id ).ownership;
}

NativeUnitOwnership_t const& UnitsState::ownership_of(
    NativeUnitId id ) const {
  return state_of( id ).ownership;
}

UnitOwnership_t& UnitsState::ownership_of( UnitId id ) {
  return state_of( id ).ownership;
}

NativeUnitOwnership_t& UnitsState::ownership_of(
    NativeUnitId id ) {
  return state_of( id ).ownership;
}

Unit const& UnitsState::unit_for( UnitId id ) const {
  return state_of( id ).unit;
}

Unit& UnitsState::unit_for( UnitId id ) {
  return state_of( id ).unit;
}

Unit const& UnitsState::euro_unit_for( GenericUnitId id ) const {
  UnitId const unit_id{ to_underlying( id ) };
  UNWRAP_CHECK( state, base::lookup( euro_units_, unit_id ) );
  CHECK( state != nullptr );
  return state->unit;
}

Unit& UnitsState::euro_unit_for( GenericUnitId id ) {
  UNWRAP_CHECK( state, base::lookup( o_.units, id ) );
  UNWRAP_CHECK( euro_state, state.get_if<UnitState::euro>() );
  return euro_state.state.unit;
}

NativeUnit const& UnitsState::unit_for( NativeUnitId id ) const {
  return state_of( id ).unit;
}

NativeUnit& UnitsState::unit_for( NativeUnitId id ) {
  return state_of( id ).unit;
}

maybe<Coord> UnitsState::maybe_coord_for( UnitId id ) const {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::world:
      return o.get<UnitOwnership::world>().coord;
    case UnitOwnership::e::free:
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::harbor:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

Coord UnitsState::coord_for( UnitId id ) const {
  UNWRAP_CHECK_MSG( coord, maybe_coord_for( id ),
                    "unit is not on map." );
  return coord;
}

maybe<Coord> UnitsState::maybe_coord_for(
    NativeUnitId id ) const {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case NativeUnitOwnership::e::world:
      return o.get<NativeUnitOwnership::world>().coord;
    case NativeUnitOwnership::e::free: return nothing;
  };
}

Coord UnitsState::coord_for( NativeUnitId id ) const {
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
    case UnitOwnership::e::harbor:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

UnitId UnitsState::holder_of( UnitId id ) const {
  UNWRAP_CHECK_MSG( holder, maybe_holder_of( id ),
                    "unit is not being held as cargo." );
  return holder;
}

maybe<UnitHarborViewState&>
UnitsState::maybe_harbor_view_state_of( UnitId id ) {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::harbor:
      return o.get<UnitOwnership::harbor>().st;
    case UnitOwnership::e::world:
    case UnitOwnership::e::free:
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

maybe<UnitHarborViewState const&>
UnitsState::maybe_harbor_view_state_of( UnitId id ) const {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::harbor:
      return o.get<UnitOwnership::harbor>().st;
    case UnitOwnership::e::world:
    case UnitOwnership::e::free:
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

UnitHarborViewState& UnitsState::harbor_view_state_of(
    UnitId id ) {
  UNWRAP_CHECK_MSG( st, maybe_harbor_view_state_of( id ),
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
      units_set.erase( GenericUnitId{ to_underlying( id ) } );
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
    case UnitOwnership::e::harbor: {
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
      break;
    }
  };
  ownership = UnitOwnership::free{};
}

void UnitsState::disown_unit( NativeUnitId id ) {
  auto& ownership = ownership_of( id );
  switch( auto& v = ownership; v.to_enum() ) {
    case NativeUnitOwnership::e::free: //
      break;
    case NativeUnitOwnership::e::world: {
      auto& [coord] = v.get<NativeUnitOwnership::world>();
      UNWRAP_CHECK( set_it,
                    base::find( units_from_coords_, coord ) );
      auto& units_set = set_it->second;
      units_set.erase( GenericUnitId{ to_underlying( id ) } );
      if( units_set.empty() ) units_from_coords_.erase( set_it );
      break;
    }
  };
  ownership = NativeUnitOwnership::free{};
}

void UnitsState::change_to_map( UnitId id, Coord target ) {
  disown_unit( id );
  units_from_coords_[target].insert(
      GenericUnitId{ to_underlying( id ) } );
  ownership_of( id ) = UnitOwnership::world{ /*coord=*/target };
}

void UnitsState::change_to_map( NativeUnitId id, Coord target ) {
  disown_unit( id );
  units_from_coords_[target].insert(
      GenericUnitId{ to_underlying( id ) } );
  ownership_of( id ) =
      NativeUnitOwnership::world{ /*coord=*/target };
}

void UnitsState::change_to_cargo_somewhere( UnitId new_holder,
                                            UnitId held,
                                            int starting_slot ) {
  auto& cargo = unit_for( new_holder ).cargo();
  for( int i = starting_slot;
       i < starting_slot + cargo.slots_total(); ++i ) {
    int modded = i % cargo.slots_total();
    if( cargo.fits( *this, Cargo::unit{ held }, modded ) ) {
      change_to_cargo( new_holder, held, modded );
      return;
    }
  }
  FATAL( "{} cannot be placed in {}'s cargo: {}",
         debug_string( unit_for( held ) ),
         debug_string( unit_for( new_holder ) ), cargo );
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
  CHECK( cargo_hold.fits( *this, Cargo::unit{ held }, slot ) );
  CHECK(
      cargo_hold.try_add( *this, Cargo::unit{ held }, slot ) );
  unit_for( held ).sentry();
  // Set new ownership
  ownership_of( held ) =
      UnitOwnership::cargo{ /*holder=*/new_holder };
}

void UnitsState::change_to_harbor_view(
    UnitId id, UnitHarborViewState info ) {
  CHECK_HAS_VALUE( check_harbor_state_invariants( info ) );
  UnitOwnership_t& ownership = ownership_of( id );
  if( !ownership.holds<UnitOwnership::harbor>() )
    disown_unit( id );
  ownership = UnitOwnership::harbor{ /*st=*/info };
}

valid_or<string> PortStatus::outbound::validate() const {
  RETURN_IF_FALSE( turns >= 0,
                   "ship outbound turn count must be larger "
                   "than zero, but instead is ",
                   turns );
  return valid;
}

valid_or<string> PortStatus::inbound::validate() const {
  RETURN_IF_FALSE( turns >= 0,
                   "ship inbound turn count must be larger than "
                   "zero, but instead is ",
                   turns );
  return valid;
}

valid_or<string> PortStatus::in_port::validate() const {
  return valid;
}

void UnitsState::change_to_colony( UnitId id, ColonyId col_id ) {
  disown_unit( id );
  worker_units_from_colony_[col_id].insert( id );
  ownership_of( id ) = UnitOwnership::colony{ col_id };
}

UnitId UnitsState::add_unit( Unit&& unit ) {
  CHECK( unit.id() == UnitId{ 0 },
         "unit ID must be zero when creating unit." );
  GenericUnitId const id      = next_unit_id();
  UnitId const        unit_id = UnitId{ to_underlying( id ) };
  unit.o_.id                  = unit_id;
  CHECK( !o_.units.contains( id ) );
  CHECK( !euro_units_.contains( unit_id ) );
  CHECK( !deleted_.contains( id ) );
  o_.units[id] = UnitState::euro{
      .state = { .unit      = std::move( unit ),
                 .ownership = UnitOwnership::free{} } };
  euro_units_[unit_id] =
      &o_.units[id].get<UnitState::euro>().state;
  return unit_id;
}

NativeUnitId UnitsState::add_unit( NativeUnit&& unit ) {
  CHECK( unit.id == NativeUnitId{ 0 },
         "unit ID must be zero when creating unit." );
  GenericUnitId const id = next_unit_id();
  NativeUnitId const  native_id =
      NativeUnitId{ to_underlying( id ) };
  unit.id = NativeUnitId{ to_underlying( id ) };
  CHECK( !o_.units.contains( id ) );
  CHECK( !native_units_.contains( native_id ) );
  CHECK( !deleted_.contains( id ) );
  o_.units[id] = UnitState::native{
      .state = { .unit      = std::move( unit ),
                 .ownership = NativeUnitOwnership::free{} } };
  native_units_[native_id] =
      &o_.units[id].get<UnitState::native>().state;
  return native_id;
}

void UnitsState::destroy_unit( UnitId unit_id ) {
  GenericUnitId const id =
      GenericUnitId{ to_underlying( unit_id ) };
  CHECK( o_.units.contains( id ) );
  CHECK( euro_units_.contains( unit_id ) );
  CHECK( !deleted_.contains( id ) );
  auto& unit = unit_for( unit_id );
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
        debug_string( unit_for( unit_id ) ) );
    cargo_units_to_destroy.push_back( cargo_unit.id );
  }
  for( UnitId to_destroy : cargo_units_to_destroy )
    destroy_unit( to_destroy );
  disown_unit( unit_id );

  o_.units.erase( id );
  euro_units_.erase( unit_id );
  deleted_.insert( id );
}

void UnitsState::destroy_unit( NativeUnitId native_id ) {
  GenericUnitId const id =
      GenericUnitId{ to_underlying( native_id ) };
  CHECK( o_.units.contains( id ) );
  CHECK( native_units_.contains( native_id ) );
  CHECK( !deleted_.contains( id ) );
  disown_unit( native_id );
  o_.units.erase( id );
  native_units_.erase( native_id );
  deleted_.insert( id );
}

bool UnitsState::exists( GenericUnitId id ) const {
  // Note that we don't consult the deleted units cache here
  // since that is only transient state. It is more robust to
  // check the next_unit_id.
  CHECK( to_underlying( id ) < to_underlying( o_.next_unit_id ),
         "unit id {} never existed.", id );
  return o_.units.contains( id );
}

bool UnitsState::exists( UnitId id ) const {
  return exists( GenericUnitId{ to_underlying( id ) } );
}

bool UnitsState::exists( NativeUnitId id ) const {
  return exists( GenericUnitId{ to_underlying( id ) } );
}

GenericUnitId UnitsState::next_unit_id() {
  GenericUnitId const curr_id = o_.next_unit_id;
  GenericUnitId const new_id =
      GenericUnitId{ to_underlying( curr_id ) + 1 };
  o_.next_unit_id = new_id;
  return curr_id;
}

GenericUnitId UnitsState::last_unit_id() const {
  CHECK( to_underlying( o_.next_unit_id ) > 0,
         "no units yet created." );
  return o_.next_unit_id;
}

unordered_set<GenericUnitId> const& UnitsState::from_coord(
    Coord const& coord ) const {
  static unordered_set<GenericUnitId> const empty = {};
  // CHECK( square_exists( c ) );
  return base::lookup( units_from_coords_, coord )
      .value_or( empty );
}

unordered_set<UnitId> const& UnitsState::from_colony(
    Colony const& colony ) const {
  UNWRAP_CHECK( units, base::lookup( worker_units_from_colony_,
                                     colony.id ) );
  return units;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::UnitsState;

  auto u = st.usertype.create<U>();

  u["last_unit_id"] = &U::last_unit_id;
  u["unit_from_id"] = []( U& o, UnitId id ) -> Unit& {
    return o.unit_for( id );
  };
};

} // namespace

} // namespace rn
