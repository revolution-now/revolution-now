/****************************************************************
**ustate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles creation, destruction, and ownership of
*              units.
*
*****************************************************************/
#include "ustate.hpp"

// Revolution Now
#include "aliases.hpp"
#include "colony.hpp"
#include "cstate.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "variant.hpp"

// Rnl
#include "rnl/ustate-impl.hpp"

// Flatbuffers
#include "fb/sg-unit_generated.h"

// base-util
#include "base-util/algo.hpp"
#include "base-util/keyval.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Unit ) {
  using StorageMap_t = unordered_map<UnitId, Unit>;
  using StatesMap_t  = unordered_map<UnitId, UnitState_t>;

  // Fields that are actually serialized.
  // clang-format off
  SAVEGAME_MEMBERS( Unit,
  ( StorageMap_t, units  ),
  ( StatesMap_t,  states ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

  // Holds deleted units for debugging purposes (they will never
  // be resurrected and their IDs will never be reused). Holding
  // the IDs here is technically redundant, but this is on pur-
  // pose in the hope that it might catch a bug.
  FlatSet<UnitId> deleted;

  // For units that are on (owned by) the world (map).
  unordered_map<Coord, FlatSet<UnitId>> units_from_coords;

  // For units that are held as cargo.
  FlatMap</*held*/ UnitId, /*holder*/ UnitId> holder_from_held;

  // For units that are held in a colony.
  unordered_map<ColonyId, FlatSet<UnitId>> units_from_colony;

private:
  SAVEGAME_FRIENDS( Unit );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    UNXP_CHECK( units.size() == states.size() );

    // Check for no free units.
    for( auto const& [id, st] : states ) {
      UNXP_CHECK( !holds<UnitState::free>( st ),
                  "unit {} is in the `free` state.", id );
    }

    // Populate units_from_coords.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::world, val ) {
        units_from_coords[val.coord].insert( id );
      }
    }

    // Populate units_from_colony.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::colony, val ) {
        units_from_colony[val.id].insert( id );
      }
    }

    // Populate holder_from_held.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::cargo, val ) {
        holder_from_held[id] = val.holder;
      }
    }

    // Check europort states.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::europort, val ) {
        XP_OR_RETURN_(
            check_europort_state_invariants( val.st ) );
      }
    }

    // Validate all unit cargos. We can only do this now after
    // all units have been loaded.
    for( auto const& p : units )
      XP_OR_RETURN_(
          p.second.cargo().check_invariants_post_load() );

    return xp_success_t{};
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return xp_success_t{}; }
};
SAVEGAME_IMPL( Unit );

UnitState_t const& unit_state( UnitId id ) {
  auto it = SG().states.find( id );
  CHECK( it != SG().states.end(), "Unit {} does not exist.",
         id );
  return it->second;
}

} // namespace

e_unit_state state_for_unit( UnitId id ) {
  auto states_it = SG().states.find( id );
  CHECK( states_it != SG().states.end() );
  switch( enum_for( states_it->second ) ) {
    case UnitState::e::free: {
      // Normal units should never be in this state but in pass-
      // ing, i.e., during creation or ownership transfer.
      SHOULD_NOT_BE_HERE;
    }
    case UnitState::e::world: return e_unit_state::world;
    case UnitState::e::cargo: return e_unit_state::cargo;
    case UnitState::e::europort: return e_unit_state::europort;
    case UnitState::e::colony: return e_unit_state::colony;
  }
}

string debug_string( UnitId id ) {
  return debug_string( unit_from_id( id ) );
}

Vec<UnitId> units_all( optional<e_nation> nation ) {
  vector<UnitId> res;
  res.reserve( SG().units.size() );
  if( nation ) {
    for( auto const& p : SG().units )
      if( *nation == p.second.nation() )
        res.push_back( p.first );
  } else {
    for( auto const& p : SG().units ) res.push_back( p.first );
  }
  return res;
}

bool unit_exists( UnitId id ) {
  bool exists  = bu::has_key( SG().units, id ).has_value();
  bool deleted = bu::has_key( SG().deleted, id ).has_value();
  if( exists )
    CHECK( !deleted, "{}: exists: {}, deleted: {}.",
           /*no debug_string*/ id, exists, deleted );
  return exists;
}

Unit& unit_from_id( UnitId id ) {
  CHECK( unit_exists( id ), "unit {} does not exist.", id );
  return val_or_die( SG().units, id );
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( tl::function_ref<void( Unit& )> func ) {
  for( auto& p : SG().units ) func( p.second );
}

// Should not be holding any references to the unit after this.
void destroy_unit( UnitId id ) {
  CHECK( unit_exists( id ) );
  CHECK( !SG().deleted.contains( id ) );
  auto& unit = unit_from_id( id );
  // Recursively destroy any units in the cargo. We must get the
  // list of units to destroy first because we don't want to
  // destroy a cargo unit while iterating over the cargo.
  vector<UnitId> cargo_units_to_destroy;
  for( auto const& cargo_id :
       unit.cargo().items_of_type<UnitId>() ) {
    lg.debug(
        "{} being destroyed as a consequence of {} being "
        "destroyed",
        debug_string( unit_from_id( cargo_id ) ),
        debug_string( unit_from_id( id ) ) );
    cargo_units_to_destroy.push_back( cargo_id );
  }
  util::map_( destroy_unit, cargo_units_to_destroy );
  internal::ustate_disown_unit( id );

  auto units_it = SG().units.find( id );
  CHECK( units_it != SG().units.end() );
  SG().units.erase( units_it->first );

  auto states_it = SG().states.find( id );
  CHECK( states_it != SG().states.end() );
  SG().states.erase( states_it->first );

  SG().deleted.insert( id );
}

UnitId create_unit( e_nation nation, e_unit_type type ) {
  Unit unit( nation, type );
  auto id = unit.id_;
  CHECK( !bu::has_key( SG().units, id ) );
  CHECK( !bu::has_key( SG().states, id ) );
  CHECK( !SG().deleted.contains( id ) );

  SG().units[id]  = move( unit );
  SG().states[id] = UnitState::free{};

  return id;
}

/****************************************************************
** Map Ownership
*****************************************************************/
FlatSet<UnitId> const& units_from_coord( Coord const& c ) {
  static FlatSet<UnitId> const empty = {};
  // CHECK( square_exists( c ) );
  auto opt_set = bu::val_safe( SG().units_from_coords, c );
  return opt_set.value_or( empty );
}

Vec<UnitId> units_from_coord_recursive( Coord coord ) {
  Vec<UnitId> res;
  for( auto id : units_from_coord( coord ) ) {
    res.push_back( id );
    auto held_units =
        unit_from_id( id ).cargo().items_of_type<UnitId>();
    for( auto held_id : held_units ) res.push_back( held_id );
  }
  return res;
}

void move_unit_from_map_to_map( UnitId id, Coord dest ) {
  CHECK( unit_exists( id ) );
  CHECK( holds<UnitState::world>( SG().states[id] ) );
  ustate_change_to_map( id, dest );
}

Vec<UnitId> units_in_rect( Rect const& rect ) {
  Vec<UnitId> res;
  for( Y i = rect.y; i < rect.y + rect.h; ++i )
    for( X j = rect.x; j < rect.x + rect.w; ++j )
      for( auto id : units_from_coord( Coord{ i, j } ) )
        res.push_back( id );
  return res;
}

Opt<Coord> coord_for_unit( UnitId id ) {
  CHECK( unit_exists( id ) );
  switch( auto& v = SG().states[id]; enum_for( v ) ) {
    case UnitState::e::free: {
      FATAL( "asking for coordinates of a free unit." );
    }
    case UnitState::e::world: {
      auto& [coord] = get_if_or_die<UnitState::world>( v );
      return coord;
    }
    case UnitState::e::cargo:
    case UnitState::e::europort:
    case UnitState::e::colony: //
      return nullopt;
  };
}

Coord coord_for_unit_indirect( UnitId id ) {
  ASSIGN_CHECK_OPT( res, coord_for_unit_indirect_safe( id ) );
  return res;
}

// If this function makes recursive calls it should always call
// the _safe variant since this function should not throw.
Opt<Coord> coord_for_unit_indirect_safe( UnitId id ) {
  CHECK( unit_exists( id ) );
  switch( auto& v = SG().states[id]; enum_for( v ) ) {
    case UnitState::e::free: {
      FATAL( "asking for coordinates of a free unit." );
    }
    case UnitState::e::world: {
      auto& [coord] = get_if_or_die<UnitState::world>( v );
      return coord;
    }
    case UnitState::e::cargo: {
      auto& [holder] = get_if_or_die<UnitState::cargo>( v );
      return coord_for_unit_indirect_safe( holder );
    }
    case UnitState::e::europort:
    case UnitState::e::colony: //
      return nullopt;
  };
}

bool is_unit_on_map_indirect( UnitId id ) {
  return coord_for_unit_indirect_safe( id ).has_value();
}

/****************************************************************
** Colony Ownership
*****************************************************************/
FlatSet<UnitId> const& units_from_colony( ColonyId id ) {
  CHECK( colony_exists( id ) );
  return SG().units_from_colony[id];
}

Opt<ColonyId> colony_for_unit_who_is_worker( UnitId id ) {
  Opt<ColonyId> res;
  if_get( unit_state( id ), UnitState::colony, colony_state ) {
    return colony_state.id;
  }
  return res;
}

bool is_unit_in_colony( UnitId id ) {
  return holds<UnitState::colony>( unit_state( id ) );
}

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nullopt otherwise.
Opt<UnitId> is_unit_onboard( UnitId id ) {
  auto opt_iter = bu::has_key( SG().holder_from_held, id );
  return opt_iter ? optional<UnitId>( ( **opt_iter ).second )
                  : nullopt;
}

/****************************************************************
** EuroPort View Ownership
*****************************************************************/
expect<> check_europort_state_invariants(
    UnitEuroPortViewState_t const& info ) {
  switch( enum_for( info ) ) {
    case UnitEuroPortViewState::e::outbound: {
      auto& [percent] =
          get_if_or_die<UnitEuroPortViewState::outbound>( info );
      UNXP_CHECK( percent >= 0.0 );
      UNXP_CHECK( percent < 1.0 );
      return xp_success_t{};
    }
    case UnitEuroPortViewState::e::inbound: {
      auto& [percent] =
          get_if_or_die<UnitEuroPortViewState::inbound>( info );
      UNXP_CHECK( percent >= 0.0 );
      UNXP_CHECK( percent < 1.0 );
      return xp_success_t{};
    }
    case UnitEuroPortViewState::e::in_port:
      return xp_success_t{};
  };
}

Opt<Ref<UnitEuroPortViewState_t>> unit_euro_port_view_info(
    UnitId id ) {
  if_get( SG().states[id], UnitState::europort, val ) {
    return val.st;
  }
  return nullopt;
}

Vec<UnitId> units_in_euro_port_view() {
  Vec<UnitId> res;
  for( auto const& [id, st] : SG().states ) {
    if( holds<UnitState::europort>( st ) ) res.push_back( id );
  }
  return res;
}

/****************************************************************
** Multi
*****************************************************************/
Opt<Coord> coord_for_unit_multi_ownership( UnitId id ) {
  if( auto maybe_map = coord_for_unit_indirect_safe( id );
      maybe_map )
    return maybe_map;
  if( auto maybe_colony = colony_for_unit_who_is_worker( id ) )
    return colony_from_id( *maybe_colony ).location();
  return nullopt;
}

/****************************************************************
** For Testing / Development Only
*****************************************************************/
UnitId create_unit_on_map( e_nation nation, e_unit_type type,
                           Coord coord ) {
  Unit& unit = unit_from_id( create_unit( nation, type ) );
  ustate_change_to_map( unit.id(), coord );
  return unit.id();
}

/****************************************************************
** Low-Level Ownership Change Functions
*****************************************************************/
void ustate_change_to_map( UnitId id, Coord const& target ) {
  internal::ustate_disown_unit( id );
  SG().units_from_coords[target].insert( id );
  SG().states[id] = UnitState::world{ /*coord=*/target };
}

void ustate_change_to_cargo( UnitId new_holder, UnitId held,
                             int slot ) {
  // Make sure that we're not adding the unit to its own cargo.
  // Should never happen theoretically, but...
  CHECK( new_holder != held );
  // Check that the proposed `held` unit cannot itself hold
  // cargo, because it is a game rule that cargo-holding units
  // cannot be cargo of other units.
  CHECK( unit_from_id( held ).desc().cargo_slots == 0 );
  // Check that the proposed `held` unit can occupy cargo.
  CHECK( unit_from_id( held )
             .desc()
             .cargo_slots_occupies.has_value() );
  auto& cargo_hold = unit_from_id( new_holder ).cargo();
  // We're clear (at least on our end).
  internal::ustate_disown_unit( held );
  // Check that there are enough open slots. Note we do this
  // after disowning the unit just in case we are moving the unit
  // into a cargo slot that it already occupies or moving a large
  // unit (i.e., one occupying multiple slots) to another slot in
  // the same cargo where it will not fit unless it is first re-
  // moved from its current slot.
  CHECK( cargo_hold.fits( held, slot ) );
  CHECK( cargo_hold.try_add( Cargo{ held }, slot ) );
  unit_from_id( held ).sentry();
  // Set new ownership
  SG().states[held] = UnitState::cargo{ /*holder=*/new_holder };
  SG().holder_from_held[held] = new_holder;
}

void ustate_change_to_cargo( UnitId new_holder, UnitId held ) {
  auto& cargo = unit_from_id( new_holder ).cargo();
  for( int i = 0; i < cargo.slots_total(); ++i ) {
    if( cargo.fits( held, i ) ) {
      ustate_change_to_cargo( new_holder, held, i );
      return;
    }
  }
  FATAL( "{} cannot be placed in {}'s cargo: {}",
         debug_string( held ), debug_string( new_holder ),
         cargo );
}

void ustate_change_to_euro_port_view(
    UnitId id, UnitEuroPortViewState_t info ) {
  CHECK_XP( check_europort_state_invariants( info ) );
  if( !holds<UnitState::europort>( SG().states[id] ) )
    internal::ustate_disown_unit( id );
  SG().states[id] = UnitState::europort{ /*st=*/info };
}

void ustate_change_to_colony( UnitId id, ColonyId col_id,
                              ColonyJob_t const& job ) {
  CHECK( unit_from_id( id ).nation() ==
         colony_from_id( col_id ).nation() );
  internal::ustate_disown_unit( id );
  SG().units_from_colony[col_id].insert( id );
  SG().states[id] = UnitState::colony{ col_id };
  colony_from_id( col_id ).add_unit( id, job );
}

/****************************************************************
** Do not call directly
*****************************************************************/
namespace internal {

// This will erase any ownership that is had over the given unit
// and mark it as free.
void ustate_disown_unit( UnitId id ) {
  switch( auto& v = SG().states[id]; enum_for( v ) ) {
    case UnitState::e::free: //
      break;
    case UnitState::e::world: {
      auto& [coord] = get_if_or_die<UnitState::world>( v );
      ASSIGN_CHECK_OPT(
          set_it, bu::has_key( SG().units_from_coords, coord ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() )
        SG().units_from_coords.erase( set_it );
      break;
    }
    case UnitState::e::cargo: {
      ASSIGN_CHECK_OPT(
          pair_it, bu::has_key( SG().holder_from_held, id ) );
      auto& holder_unit = unit_from_id( pair_it->second );
      ASSIGN_CHECK_OPT( slot_idx,
                        holder_unit.cargo().find_unit( id ) );
      holder_unit.cargo().remove( slot_idx );
      SG().holder_from_held.erase( pair_it );
      break;
    }
    case UnitState::e::europort: {
      // Ensure the unit has no units in its cargo.
      CHECK( unit_from_id( id )
                 .cargo()
                 .count_items_of_type<UnitId>() == 0 );
      break;
    }
    case UnitState::e::colony: {
      auto& val    = get_if_or_die<UnitState::colony>( v );
      auto  col_id = val.id;
      ASSIGN_CHECK_OPT(
          set_it,
          bu::has_key( SG().units_from_colony, col_id ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() )
        SG().units_from_colony.erase( set_it );
      colony_from_id( col_id ).remove_unit( id );
      break;
    }
  };
  SG().states[id] = UnitState::free{};
}

} // namespace internal

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_on_map, Unit const&, e_nation nation,
        e_unit_type type, Coord const& coord ) {
  auto id = create_unit_on_map( nation, type, coord );
  lg.info( "created a {} on square {}.", unit_desc( type ).name,
           coord );
  return unit_from_id( id );
}

LUA_FN( unit_from_id, Unit const&, UnitId id ) {
  return unit_from_id( id );
}

LUA_FN( coord_for_unit, Coord, UnitId id ) {
  // FIXME: try to return Opt<Coord> here which would automati-
  // cally convert to nil when the result is nullopt.
  auto maybe_coord = coord_for_unit( id );
  CHECK( maybe_coord.has_value(), "Unit {} is not on the map.",
         id );
  return *maybe_coord;
}

} // namespace

} // namespace rn
