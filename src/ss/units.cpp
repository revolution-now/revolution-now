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

// config
#include "config/unit-type.rds.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/logger.hpp"
#include "base/to-str-ext-std.hpp"
#include "base/variant-util.hpp"

// TODO: This is temporary while we migrate any existing save
// files to adhere to the new ordering map constraints.
#define WRITE_ORDERING_MAP 0

using namespace std;

namespace rn {

namespace {

constexpr GenericUnitId kFirstUnitId{ 1 };

// FIXME: we should have a generic way to do this.
valid_or<generic_err> check_harbor_state_invariants(
    PortStatus const& port_status ) {
  valid_or<string> res =
      std::visit( []( auto const& o ) { return o.validate(); },
                  port_status.as_base() );
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
        UnitOwnership const& st = o.ownership;
        REFL_VALIDATE( !st.holds<UnitOwnership::free>(),
                       "unit {} is in the `free` state.", id );
        break;
      }
      case UnitState::e::native:
        break;
    }
  }

  // Check that ships in the harbor have cleared orders. This is
  // important because the game does not provide any mechanism
  // for the player to clear their orders when in the harbor, and
  // so they will never move.
  for( auto const& [id, unit_state] : units ) {
    SWITCH( unit_state ) {
      CASE( euro ) {
        UnitOwnership const& st = euro.ownership;
        if( !euro.unit.desc().ship ) break;
        maybe<UnitOwnership::harbor const&> harbor =
            st.get_if<UnitOwnership::harbor>();
        if( !harbor.has_value() ) break;
        if( !harbor->port_status.holds<PortStatus::in_port>() )
          break;
        if( euro.unit.orders().holds<unit_orders::damaged>() )
          break;
        REFL_VALIDATE(
            euro.unit.orders().holds<unit_orders::none>(),
            "unit {} in port is not damaged but does not have "
            "cleared orders.",
            id );
        break;
      }
      CASE( native ) { break; }
    }
  }

  // Check that the indices in the ordering map are valid.
  unordered_set<int64_t> used_ordering_indices;
  for( auto const& [unit_id, index] : unit_ordering ) {
    REFL_VALIDATE( index > 0,
                   "unit {} has an ordering index less than or "
                   "equal to zero which is not allowed.",
                   unit_id );
    REFL_VALIDATE(
        index <= curr_unit_ordering_index,
        "unit {} has an ordering index greater than "
        "curr_unit_ordering_index which is not allowed.",
        unit_id );
    REFL_VALIDATE( !used_ordering_indices.contains( index ),
                   "unit ordering index {} appears in ordering "
                   "map multiple times.",
                   index );
    used_ordering_indices.insert( index );
  }

  // Check that all units in the ordering map 1) exist and 2) are
  // owned by either the map or the harbor.
  for( auto const& [unit_id, _] : unit_ordering ) {
    auto const iter = units.find( unit_id );
    REFL_VALIDATE(
        iter != units.end(),
        "unit {} is in the ordering map but does not exist.",
        unit_id );
    auto const& [__, state] = *iter;
    SWITCH( state ) {
      CASE( euro ) {
        bool const correct_ownership =
            euro.ownership.holds<UnitOwnership::world>() ||
            euro.ownership.holds<UnitOwnership::harbor>();
        REFL_VALIDATE( correct_ownership,
                       "unit {} is in the ordering map but is "
                       "not owned by either the map or the "
                       "harbor, instead its ownership is {}.",
                       unit_id, euro.ownership );
        break;
      }
      CASE( native ) {}
    }
  }

#if !WRITE_ORDERING_MAP
  // Check that all units owned by the map or the harbor are
  // present in the ordering map.
  for( auto const& [id, unit_state] : units ) {
    SWITCH( unit_state ) {
      CASE( euro ) {
        UnitId const unit_id = euro.unit.id();
        SWITCH( euro.ownership ) {
          CASE( cargo ) { break; }
          CASE( colony ) { break; }
          CASE( dwelling ) { break; }
          CASE( free ) { break; }
          CASE( harbor ) {
            REFL_VALIDATE(
                unit_ordering.contains( unit_id ),
                "unit {} is in the harbor but does not have an "
                "entry in the unit ordering map.",
                unit_id );
            break;
          }
          CASE( world ) {
            REFL_VALIDATE(
                unit_ordering.contains( unit_id ),
                "unit {} is on the map but does not have an "
                "entry in the unit ordering map.",
                unit_id );
            break;
          }
        }
        break;
      }
      CASE( native ) { break; }
    }
  }
#endif

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
        REFL_VALIDATE( o.unit.cargo().validate( *this ) );
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
        UnitOwnership const& st = o.ownership;
        if_get( st, UnitOwnership::world, val ) {
          units_from_coords_[val.coord].insert( id );
        }
        break;
      }
      case UnitState::e::native: {
        auto& o = unit_state.get<UnitState::native>();
        NativeUnitOwnership const& ownership = o.ownership;
        units_from_coords_[ownership.coord].insert( id );
        break;
      }
    }
  }

  // Populate worker_units_from_colony_.
  for( auto const& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        UnitOwnership const& st = o.ownership;
        if_get( st, UnitOwnership::colony, val ) {
          worker_units_from_colony_[val.id].insert(
              UnitId{ to_underlying( id ) } );
        }
        break;
      }
      case UnitState::e::native:
        break;
    }
  }

  // Populate missionary_in_dwelling_.
  for( auto const& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        UnitOwnership const& st = o.ownership;
        if_get( st, UnitOwnership::dwelling, val ) {
          CHECK(
              !missionary_in_dwelling_.contains( val.id ),
              "multiple missionary units in village with id {}.",
              val.id );
          missionary_in_dwelling_[val.id] =
              UnitId{ to_underlying( id ) };
        }
        break;
      }
      case UnitState::e::native:
        break;
    }
  }

  // Populate braves_for_dwelling_.
  for( auto const& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::native: {
        auto& o = unit_state.get<UnitState::native>();
        NativeUnitOwnership const& ownership = o.ownership;
        CHECK(
            !braves_for_dwelling_.contains(
                ownership.dwelling_id ),
            "multiple native units (id={} and id={}) have the "
            "same dwelling_id={}.",
            id, braves_for_dwelling_[ownership.dwelling_id],
            ownership.dwelling_id );
        braves_for_dwelling_[ownership.dwelling_id] = {
          NativeUnitId{ to_underlying( id ) } };
        break;
      }
      case UnitState::e::euro:
        break;
    }
  }

  // Populate unit kinds.
  for( auto& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto& o = unit_state.get<UnitState::euro>();
        euro_units_[UnitId{ to_underlying( id ) }] = &o;
        break;
      }
      case UnitState::e::native: {
        auto& o = unit_state.get<UnitState::native>();
        native_units_[NativeUnitId{ to_underlying( id ) }] = &o;
        break;
      }
    }
  }

#if WRITE_ORDERING_MAP
  // TODO: temporary. Add any missing map/harbor units into the
  // ordering map so that we can load existing saves. Remove this
  // once all existing saves have been migrated.
  for( auto& [id, unit_state] : o_.units ) {
    switch( unit_state.to_enum() ) {
      case UnitState::e::euro: {
        auto const& o = unit_state.get<UnitState::euro>();
        auto const& ownership = o.ownership;
        bool const needs_ordering =
            ownership.holds<UnitOwnership::world>() ||
            ownership.holds<UnitOwnership::harbor>();
        UnitId const unit_id{ to_underlying( id ) };
        if( needs_ordering &&
            !o_.unit_ordering.contains( unit_id ) )
          add_or_bump_unit_ordering_index( unit_id );
        break;
      }
      case UnitState::e::native:
        break;
    }
  }
#endif
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

unordered_map<GenericUnitId, UnitState> const& UnitsState::all()
    const {
  return o_.units;
}

unordered_map<UnitId, UnitState::euro const*> const&
UnitsState::euro_all() const {
  return euro_units_;
}

unordered_map<NativeUnitId, UnitState::native const*> const&
UnitsState::native_all() const {
  return native_units_;
}

UnitState const& UnitsState::state_of( GenericUnitId id ) const {
  CHECK( !deleted_.contains( id ),
         "unit with ID {} existed but was deleted.", id );
  UNWRAP_CHECK_MSG( unit_state, base::lookup( o_.units, id ),
                    "unit {} does not exist.", id );
  return unit_state;
}

UnitState& UnitsState::state_of( GenericUnitId id ) {
  CHECK( !deleted_.contains( id ),
         "unit with ID {} existed but was deleted.", id );
  UNWRAP_CHECK_MSG( unit_state, base::lookup( o_.units, id ),
                    "unit {} does not exist.", id );
  return unit_state;
}

UnitState::euro const& UnitsState::state_of( UnitId id ) const {
  UNWRAP_CHECK_MSG( unit_state, base::lookup( euro_units_, id ),
                    "unit {} does not exist.", id );
  return *unit_state;
}

UnitState::native const& UnitsState::state_of(
    NativeUnitId id ) const {
  UNWRAP_CHECK_MSG( unit_state,
                    base::lookup( native_units_, id ),
                    "unit {} does not exist.", id );
  return *unit_state;
}

UnitState::euro& UnitsState::state_of( UnitId id ) {
  UnitState& generic_state =
      state_of( GenericUnitId{ to_underlying( id ) } );
  UNWRAP_CHECK( euro_unit_state,
                generic_state.get_if<UnitState::euro>() );
  return euro_unit_state;
}

UnitState::native& UnitsState::state_of( NativeUnitId id ) {
  UnitState& generic_state =
      state_of( GenericUnitId{ to_underlying( id ) } );
  UNWRAP_CHECK( native_unit_state,
                generic_state.get_if<UnitState::native>() );
  return native_unit_state;
}

UnitOwnership const& UnitsState::ownership_of(
    UnitId id ) const {
  return state_of( id ).ownership;
}

NativeUnitOwnership const& UnitsState::ownership_of(
    NativeUnitId id ) const {
  return state_of( id ).ownership;
}

UnitOwnership& UnitsState::ownership_of( UnitId id ) {
  return state_of( id ).ownership;
}

NativeUnitOwnership& UnitsState::ownership_of(
    NativeUnitId id ) {
  return state_of( id ).ownership;
}

Unit const& UnitsState::unit_for( UnitId id ) const {
  return state_of( id ).unit;
}

Unit& UnitsState::unit_for( UnitId id ) {
  return state_of( id ).unit;
}

base::maybe<Unit const&> UnitsState::maybe_euro_unit_for(
    GenericUnitId id ) const {
  UnitId const unit_id{ to_underlying( id ) };
  maybe<UnitState::euro const*> const state =
      base::lookup( euro_units_, unit_id );
  if( !state.has_value() ) return nothing;
  CHECK( *state != nullptr );
  return ( *state )->unit;
}

base::maybe<Unit&> UnitsState::maybe_euro_unit_for(
    GenericUnitId id ) {
  UNWRAP_CHECK( state, base::lookup( o_.units, id ) );
  maybe<UnitState::euro&> euro = state.get_if<UnitState::euro>();
  if( !euro.has_value() ) return nothing;
  return euro->unit;
}

Unit const& UnitsState::euro_unit_for( GenericUnitId id ) const {
  UNWRAP_CHECK( res, maybe_euro_unit_for( id ) );
  return res;
}

Unit& UnitsState::euro_unit_for( GenericUnitId id ) {
  UNWRAP_CHECK( res, maybe_euro_unit_for( id ) );
  return res;
}

NativeUnit const& UnitsState::native_unit_for(
    GenericUnitId id ) const {
  NativeUnitId const native_unit_id{ to_underlying( id ) };
  UNWRAP_CHECK( state,
                base::lookup( native_units_, native_unit_id ) );
  CHECK( state != nullptr );
  return state->unit;
}

NativeUnit& UnitsState::native_unit_for( GenericUnitId id ) {
  UNWRAP_CHECK( state, base::lookup( o_.units, id ) );
  UNWRAP_CHECK( native_state,
                state.get_if<UnitState::native>() );
  return native_state.unit;
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
    case UnitOwnership::e::colony:
    case UnitOwnership::e::dwelling: //
      return nothing;
  };
}

Coord UnitsState::coord_for( UnitId id ) const {
  UNWRAP_CHECK_MSG( coord, maybe_coord_for( id ),
                    "unit is not on map." );
  return coord;
}

Coord UnitsState::coord_for( NativeUnitId id ) const {
  return ownership_of( id ).coord;
}

maybe<Coord> UnitsState::maybe_coord_for(
    GenericUnitId id ) const {
  switch( unit_kind( id ) ) {
    case e_unit_kind::euro:
      return maybe_coord_for( check_euro_unit( id ) );
    case e_unit_kind::native:
      return coord_for( check_native_unit( id ) );
  }
}

Coord UnitsState::coord_for( GenericUnitId id ) const {
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
    case UnitOwnership::e::colony:
    case UnitOwnership::e::dwelling: //
      return nothing;
  };
}

UnitId UnitsState::holder_of( UnitId id ) const {
  UNWRAP_CHECK_MSG( holder, maybe_holder_of( id ),
                    "unit is not being held as cargo." );
  return holder;
}

DwellingId UnitsState::dwelling_for( NativeUnitId id ) const {
  return ownership_of( id ).dwelling_id;
}

maybe<DwellingId> UnitsState::maybe_dwelling_for_missionary(
    UnitId id ) const {
  switch( auto& o = ownership_of( id ); o.to_enum() ) {
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::world:
    case UnitOwnership::e::free:
    case UnitOwnership::e::harbor:
    case UnitOwnership::e::colony: //
      return nothing;
    case UnitOwnership::e::dwelling: //
      return o.get<UnitOwnership::dwelling>().id;
  };
}

maybe<UnitOwnership::harbor&>
UnitsState::maybe_harbor_view_state_of( UnitId id ) {
  return ownership_of( id ).get_if<UnitOwnership::harbor>();
}

maybe<UnitOwnership::harbor const&>
UnitsState::maybe_harbor_view_state_of( UnitId id ) const {
  return ownership_of( id ).get_if<UnitOwnership::harbor>();
}

UnitOwnership::harbor& UnitsState::harbor_view_state_of(
    UnitId id ) {
  UNWRAP_CHECK_MSG( st, maybe_harbor_view_state_of( id ),
                    "unit is not in the old world state." );
  return st;
}

void UnitsState::bump_unit_ordering( UnitId const id ) {
  CHECK( o_.unit_ordering.contains( id ),
         "unit {} was asked to bump its ordering index but it "
         "is not in the ordering map.",
         id );
  add_or_bump_unit_ordering_index( id );
}

void UnitsState::add_or_bump_unit_ordering_index(
    UnitId const id ) {
  o_.unit_ordering.erase( id );
  ++o_.curr_unit_ordering_index;
  o_.unit_ordering[id] = o_.curr_unit_ordering_index;
}

int64_t UnitsState::unit_ordering( UnitId const id ) const {
  auto const iter = o_.unit_ordering.find( id );
  CHECK( iter != o_.unit_ordering.end(),
         "unit {} was asked to bump its ordering index but it "
         "is not in the ordering map.",
         id );
  return iter->second;
}

void UnitsState::move_unit_on_map( NativeUnitId id,
                                   Coord target ) {
  auto& [curr_coord, dwelling_id] = ownership_of( id );
  CHECK( braves_for_dwelling_.contains( dwelling_id ) );
  CHECK( braves_for_dwelling_[dwelling_id].contains( id ) );
  // Now remove it from its current position.
  auto set_it = base::find( units_from_coords_, curr_coord );
  CHECK( set_it != units_from_coords_.end() );
  auto& units_set = set_it->second;
  units_set.erase( GenericUnitId{ to_underlying( id ) } );
  if( units_set.empty() ) units_from_coords_.erase( set_it );
  // And add it to the new location.
  units_from_coords_[target].insert(
      GenericUnitId{ to_underlying( id ) } );
  curr_coord = target;
}

void UnitsState::disown_unit( UnitId const id ) {
  auto& ownership = ownership_of( id );
  o_.unit_ordering.erase( id );
  switch( auto& v = ownership; v.to_enum() ) {
    case UnitOwnership::e::free: //
      break;
    case UnitOwnership::e::world: {
      auto& [coord] = v.get<UnitOwnership::world>();
      auto set_it   = base::find( units_from_coords_, coord );
      CHECK( set_it != units_from_coords_.end() );
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
      auto& val   = v.get<UnitOwnership::colony>();
      auto col_id = val.id;
      auto set_it =
          base::find( worker_units_from_colony_, col_id );
      CHECK( set_it != worker_units_from_colony_.end() );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() )
        worker_units_from_colony_.erase( set_it );
      break;
    }
    case UnitOwnership::e::dwelling: {
      auto& val = v.get<UnitOwnership::dwelling>();
      DwellingId const dwelling_id = val.id;
      CHECK( missionary_in_dwelling_.contains( dwelling_id ) );
      missionary_in_dwelling_.erase( dwelling_id );
      break;
    }
  };
  ownership = UnitOwnership::free{};
}

void UnitsState::change_to_map( UnitId id, Coord target ) {
  disown_unit( id );
  units_from_coords_[target].insert(
      GenericUnitId{ to_underlying( id ) } );
  ownership_of( id ) = UnitOwnership::world{ /*coord=*/target };
  add_or_bump_unit_ordering_index( id );
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
    UnitId id, PortStatus const& port_status,
    maybe<Coord> sailed_from ) {
  CHECK_HAS_VALUE(
      check_harbor_state_invariants( port_status ) );
  UnitOwnership& ownership = ownership_of( id );
  if( !ownership.holds<UnitOwnership::harbor>() )
    disown_unit( id );
  ownership = UnitOwnership::harbor{
    .port_status = port_status, .sailed_from = sailed_from };
  add_or_bump_unit_ordering_index( id );
}

void UnitsState::change_to_dwelling( UnitId unit_id,
                                     DwellingId dwelling_id ) {
  disown_unit( unit_id );
  CHECK( !missionary_in_dwelling_.contains( dwelling_id ) );
  missionary_in_dwelling_[dwelling_id] = unit_id;
  ownership_of( unit_id ) =
      UnitOwnership::dwelling{ .id = dwelling_id };
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
  GenericUnitId const id = next_unit_id();
  UnitId const unit_id   = UnitId{ to_underlying( id ) };
  unit.o_.id             = unit_id;
  CHECK( !o_.units.contains( id ) );
  CHECK( !euro_units_.contains( unit_id ) );
  CHECK( !deleted_.contains( id ) );
  o_.units[id] =
      UnitState::euro{ .unit      = std::move( unit ),
                       .ownership = UnitOwnership::free{} };
  euro_units_[unit_id] = &o_.units[id].get<UnitState::euro>();
  return unit_id;
}

NativeUnitId UnitsState::add_unit_on_map(
    NativeUnit&& unit, Coord target, DwellingId dwelling_id ) {
  CHECK( unit.id == NativeUnitId{ 0 },
         "unit ID must be zero when creating unit." );
  GenericUnitId const id = next_unit_id();
  NativeUnitId const native_id =
      NativeUnitId{ to_underlying( id ) };
  unit.id = NativeUnitId{ to_underlying( id ) };
  CHECK( !o_.units.contains( id ) );
  CHECK( !native_units_.contains( native_id ) );
  CHECK( !deleted_.contains( id ) );
  o_.units[id] = UnitState::native{
    .unit      = std::move( unit ),
    .ownership = NativeUnitOwnership{
      .coord = target, .dwelling_id = dwelling_id } };
  native_units_[native_id] =
      &o_.units[id].get<UnitState::native>();
  units_from_coords_[target].insert( id );
  // Note: this dwelling may already have a brave associated with
  // it, even though the game only allows one active brave per
  // dwelling. This is because sometimes temporary braves are
  // created to act as visual targets when attacking a dwelling.
  braves_for_dwelling_[dwelling_id].insert( native_id );
  return native_id;
}

void UnitsState::destroy_unit( UnitId unit_id ) {
  GenericUnitId const id =
      GenericUnitId{ to_underlying( unit_id ) };
  CHECK( o_.units.contains( id ) );
  CHECK( euro_units_.contains( unit_id ) );
  CHECK( !deleted_.contains( id ) );
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

  auto& [coord, dwelling_id] = ownership_of( native_id );
  // First remove the unit's dwelling association.
  CHECK( braves_for_dwelling_.contains( dwelling_id ) );
  CHECK(
      braves_for_dwelling_[dwelling_id].contains( native_id ) );
  braves_for_dwelling_[dwelling_id].erase( native_id );
  // Now remove it from the map.
  auto set_it = base::find( units_from_coords_, coord );
  CHECK( set_it != units_from_coords_.end() );
  auto& units_set = set_it->second;
  units_set.erase( GenericUnitId{ to_underlying( id ) } );
  if( units_set.empty() ) units_from_coords_.erase( set_it );

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

std::unordered_set<GenericUnitId> const& UnitsState::from_coord(
    gfx::point const tile ) const {
  static unordered_set<GenericUnitId> const empty = {};
  // CHECK( square_exists( c ) );
  return base::lookup( units_from_coords_,
                       Coord::from_gfx( tile ) )
      .value_or( empty );
}

unordered_set<GenericUnitId> const& UnitsState::from_coord(
    Coord const& coord ) const {
  return from_coord( coord.to_gfx() );
}

unordered_map<Coord, unordered_set<GenericUnitId>> const&
UnitsState::from_coords() const {
  return units_from_coords_;
}

unordered_set<UnitId> const& UnitsState::from_colony(
    Colony const& colony ) const {
  // The empty case can happen during testing when there haven't
  // been any workers added to a colony, but should not happen
  // during a normal game.
  static unordered_set<UnitId> const empty;
  auto const units =
      base::lookup( worker_units_from_colony_, colony.id );
  return units.value_or( empty );
}

maybe<UnitId> UnitsState::missionary_from_dwelling(
    DwellingId dwelling_id ) const {
  return base::lookup( missionary_in_dwelling_, dwelling_id );
}

unordered_set<NativeUnitId> const&
UnitsState::braves_for_dwelling( DwellingId dwelling_id ) const {
  static unordered_set<NativeUnitId> empty;
  maybe<unordered_set<NativeUnitId> const&> units =
      base::lookup( braves_for_dwelling_, dwelling_id );
  return units.has_value() ? *units : empty;
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
  u["from_coord"] = [&]( U& o, Coord tile ) -> lua::table {
    lua::table tbl   = st.table.create();
    auto& from_coord = o.from_coord( tile );
    int i            = 1;
    for( auto id : from_coord ) tbl[i++] = id;
    return tbl;
  };
};

} // namespace

} // namespace rn
