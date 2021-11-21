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
#include "colony.hpp"
#include "cstate.hpp"
#include "error.hpp"
#include "fb.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "sg-macros.hpp"
#include "variant.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"

// Rds
#include "rds/ustate-impl.hpp"

// Flatbuffers
#include "fb/sg-unit_generated.h"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Unit );

namespace {

using ::base::function_ref;

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
  unordered_set<UnitId> deleted;

  // For units that are on (owned by) the world (map).
  unordered_map<Coord, unordered_set<UnitId>> units_from_coords;

  // For units that are held as cargo.
  unordered_map</*held*/ UnitId, /*holder*/ UnitId>
      holder_from_held;

  // For units that are held in a colony.
  unordered_map<ColonyId, unordered_set<UnitId>>
      worker_units_from_colony;

 private:
  SAVEGAME_FRIENDS( Unit );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    TRUE_OR_RETURN_GENERIC_ERR( units.size() == states.size() );

    // Check for no free units.
    for( auto const& [id, st] : states ) {
      TRUE_OR_RETURN_GENERIC_ERR(
          !holds<UnitState::free>( st ),
          "unit {} is in the `free` state.", id );
    }

    // Populate units_from_coords.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::world, val ) {
        units_from_coords[val.coord].insert( id );
      }
    }

    // Populate worker_units_from_colony.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::colony, val ) {
        worker_units_from_colony[val.id].insert( id );
      }
    }

    // Populate holder_from_held.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::cargo, val ) {
        holder_from_held[id] = val.holder;
      }
    }

    // Check old_world states.
    for( auto const& [id, st] : states ) {
      if_get( st, UnitState::old_world, val ) {
        HAS_VALUE_OR_RET(
            check_old_world_state_invariants( val.st ) );
      }
    }

    // Validate all unit cargos. We can only do this now after
    // all units have been loaded.
    for( auto const& p : units )
      HAS_VALUE_OR_RET(
          p.second.cargo().check_invariants_post_load() );

    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return valid; }
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
  switch( states_it->second.to_enum() ) {
    case UnitState::e::free: {
      // Normal units should never be in this state but in pass-
      // ing, i.e., during creation or ownership transfer.
      SHOULD_NOT_BE_HERE;
    }
    case UnitState::e::world: return e_unit_state::world;
    case UnitState::e::cargo: return e_unit_state::cargo;
    case UnitState::e::old_world: return e_unit_state::old_world;
    case UnitState::e::colony: return e_unit_state::colony;
  }
}

string debug_string( UnitId id ) {
  return debug_string( unit_from_id( id ) );
}

vector<UnitId> units_all( maybe<e_nation> nation ) {
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
  bool exists  = SG().units.contains( id );
  bool deleted = SG().deleted.contains( id );
  if( exists )
    CHECK( !deleted, "{}: exists: {}, deleted: {}.",
           /*no debug_string*/ id, exists, deleted );
  return exists;
}

Unit& unit_from_id( UnitId id ) {
  auto maybe_id = base::lookup( SG().units, id );
  CHECK( maybe_id, "unit {} does not exist.", id );
  return *maybe_id;
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( function_ref<void( Unit& )> func ) {
  for( auto& p : SG().units ) func( p.second );
}

void map_units( e_nation                    nation,
                function_ref<void( Unit& )> func ) {
  for( auto& p : SG().units )
    if( p.second.nation() == nation ) func( p.second );
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

UnitId create_unit( e_nation nation, UnitComposition comp ) {
  Unit unit( nation, std::move( comp ) );
  auto id = unit.id_;
  CHECK( !SG().units.contains( id ) );
  CHECK( !SG().states.contains( id ) );
  CHECK( !SG().deleted.contains( id ) );

  SG().units[id]  = move( unit );
  SG().states[id] = UnitState::free{};

  return id;
}

UnitId create_unit( e_nation nation, UnitType type ) {
  return create_unit( nation, UnitComposition::create( type ) );
}

/****************************************************************
** Map Ownership
*****************************************************************/
unordered_set<UnitId> const& units_from_coord( Coord const& c ) {
  static unordered_set<UnitId> const empty = {};
  // CHECK( square_exists( c ) );
  return base::lookup( as_const( SG().units_from_coords ), c )
      .value_or( empty );
}

vector<UnitId> units_from_coord_recursive( Coord coord ) {
  vector<UnitId> res;
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

vector<UnitId> units_in_rect( Rect const& rect ) {
  vector<UnitId> res;
  for( Y i = rect.y; i < rect.y + rect.h; ++i )
    for( X j = rect.x; j < rect.x + rect.w; ++j )
      for( auto id : units_from_coord( Coord{ i, j } ) )
        res.push_back( id );
  return res;
}

vector<UnitId> surrounding_units( Coord const& coord ) {
  vector<UnitId> res;
  for( e_direction d : enum_traits<e_direction>::values ) {
    if( d == e_direction::c ) continue;
    for( auto id : units_from_coord( coord.moved( d ) ) )
      res.push_back( id );
  }
  return res;
}

maybe<Coord> coord_for_unit( UnitId id ) {
  CHECK( unit_exists( id ) );
  switch( auto& v = SG().states[id]; v.to_enum() ) {
    case UnitState::e::free: {
      FATAL( "asking for coordinates of a free unit." );
    }
    case UnitState::e::world: {
      auto& [coord] = v.get<UnitState::world>();
      return coord;
    }
    case UnitState::e::cargo:
    case UnitState::e::old_world:
    case UnitState::e::colony: //
      return nothing;
  };
}

Coord coord_for_unit_indirect_or_die( UnitId id ) {
  UNWRAP_CHECK( res, coord_for_unit_indirect( id ) );
  return res;
}

// If this function makes recursive calls it should always call
// the _safe variant since this function should not throw.
maybe<Coord> coord_for_unit_indirect( UnitId id ) {
  CHECK( unit_exists( id ) );
  switch( auto& v = SG().states[id]; v.to_enum() ) {
    case UnitState::e::free: {
      FATAL( "asking for coordinates of a free unit." );
    }
    case UnitState::e::world: {
      auto& [coord] = v.get<UnitState::world>();
      return coord;
    }
    case UnitState::e::cargo: {
      auto& [holder] = v.get<UnitState::cargo>();
      return coord_for_unit_indirect( holder );
    }
    case UnitState::e::old_world:
    case UnitState::e::colony: //
      return nothing;
  };
}

bool is_unit_on_map_indirect( UnitId id ) {
  return coord_for_unit_indirect( id ).has_value();
}

bool is_unit_on_map( UnitId id ) {
  return state_for_unit( id ) == e_unit_state::world;
}

/****************************************************************
** Colony Ownership
*****************************************************************/
unordered_set<UnitId> const& worker_units_from_colony(
    ColonyId id ) {
  CHECK( colony_exists( id ) );
  return SG().worker_units_from_colony[id];
}

unordered_set<UnitId> units_at_or_in_colony( ColonyId id ) {
  CHECK( colony_exists( id ) );
  unordered_set<UnitId> all = worker_units_from_colony( id );
  Coord colony_loc          = colony_from_id( id ).location();
  for( UnitId map_id : units_from_coord( colony_loc ) )
    all.insert( map_id );
  return all;
}

maybe<ColonyId> colony_for_unit_who_is_worker( UnitId id ) {
  maybe<ColonyId> res;
  if_get( unit_state( id ), UnitState::colony, colony_state ) {
    return colony_state.id;
  }
  return res;
}

bool is_unit_in_colony( UnitId id ) {
  return holds<UnitState::colony>( unit_state( id ) )
      .has_value();
}

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitId id ) {
  return base::lookup( SG().holder_from_held, id );
}

/****************************************************************
** Old World View Ownership
*****************************************************************/
valid_or<generic_err> check_old_world_state_invariants(
    UnitOldWorldViewState_t const& info ) {
  switch( info.to_enum() ) {
    case UnitOldWorldViewState::e::outbound: {
      auto& [percent] =
          info.get<UnitOldWorldViewState::outbound>();
      TRUE_OR_RETURN_GENERIC_ERR( percent >= 0.0 );
      TRUE_OR_RETURN_GENERIC_ERR( percent <= 1.0 );
      return valid;
    }
    case UnitOldWorldViewState::e::inbound: {
      auto& [percent] =
          info.get<UnitOldWorldViewState::inbound>();
      TRUE_OR_RETURN_GENERIC_ERR( percent >= 0.0 );
      TRUE_OR_RETURN_GENERIC_ERR( percent <= 1.0 );
      return valid;
    }
    case UnitOldWorldViewState::e::in_port: return valid;
  };
}

maybe<UnitOldWorldViewState_t&> unit_old_world_view_info(
    UnitId id ) {
  if_get( SG().states[id], UnitState::old_world, val ) {
    return val.st;
  }
  return nothing;
}

vector<UnitId> units_in_old_world_view() {
  vector<UnitId> res;
  for( auto const& [id, st] : SG().states ) {
    if( holds<UnitState::old_world>( st ) ) res.push_back( id );
  }
  return res;
}

/****************************************************************
** Multi
*****************************************************************/
maybe<Coord> coord_for_unit_multi_ownership( UnitId id ) {
  if( auto maybe_map = coord_for_unit_indirect( id ); maybe_map )
    return maybe_map;
  if( auto maybe_colony = colony_for_unit_who_is_worker( id ) )
    return colony_from_id( *maybe_colony ).location();
  return nothing;
}

/****************************************************************
** For Testing / Development Only
*****************************************************************/
UnitId create_unit_on_map( e_nation nation, UnitComposition comp,
                           Coord coord ) {
  Unit& unit =
      unit_from_id( create_unit( nation, std::move( comp ) ) );
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

void ustate_change_to_cargo_somewhere( UnitId new_holder,
                                       UnitId held,
                                       int    starting_slot ) {
  auto& cargo = unit_from_id( new_holder ).cargo();
  for( int i = starting_slot;
       i < starting_slot + cargo.slots_total(); ++i ) {
    int modded = i % cargo.slots_total();
    if( cargo.fits( held, modded ) ) {
      ustate_change_to_cargo( new_holder, held, modded );
      return;
    }
  }
  FATAL( "{} cannot be placed in {}'s cargo: {}",
         debug_string( held ), debug_string( new_holder ),
         cargo );
}

void ustate_change_to_old_world_view(
    UnitId id, UnitOldWorldViewState_t info ) {
  CHECK_HAS_VALUE( check_old_world_state_invariants( info ) );
  if( !holds<UnitState::old_world>( SG().states[id] ) )
    internal::ustate_disown_unit( id );
  SG().states[id] = UnitState::old_world{ /*st=*/info };
}

void ustate_change_to_colony( UnitId id, ColonyId col_id,
                              ColonyJob_t const& job ) {
  CHECK( unit_from_id( id ).nation() ==
         colony_from_id( col_id ).nation() );
  internal::ustate_disown_unit( id );
  SG().worker_units_from_colony[col_id].insert( id );
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
  switch( auto& v = SG().states[id]; v.to_enum() ) {
    case UnitState::e::free: //
      break;
    case UnitState::e::world: {
      auto& [coord] = v.get<UnitState::world>();
      UNWRAP_CHECK(
          set_it, base::find( SG().units_from_coords, coord ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() )
        SG().units_from_coords.erase( set_it );
      break;
    }
    case UnitState::e::cargo: {
      UNWRAP_CHECK( pair_it,
                    base::find( SG().holder_from_held, id ) );
      auto& holder_unit = unit_from_id( pair_it->second );
      UNWRAP_CHECK( slot_idx,
                    holder_unit.cargo().find_unit( id ) );
      holder_unit.cargo().remove( slot_idx );
      SG().holder_from_held.erase( pair_it );
      break;
    }
    case UnitState::e::old_world: {
      break;
    }
    case UnitState::e::colony: {
      auto& val    = v.get<UnitState::colony>();
      auto  col_id = val.id;
      UNWRAP_CHECK(
          set_it,
          base::find( SG().worker_units_from_colony, col_id ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() )
        SG().worker_units_from_colony.erase( set_it );
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

LUA_FN( create_unit_on_map, Unit&, e_nation nation,
        UnitComposition& comp, Coord const& coord ) {
  auto id = create_unit_on_map( nation, comp, coord );
  lg.info( "created a {} on square {}.",
           unit_attr( comp.type() ).name, coord );
  return unit_from_id( id );
}

LUA_FN( unit_from_id, Unit&, UnitId id ) {
  return unit_from_id( id );
}

LUA_FN( coord_for_unit, maybe<Coord>, UnitId id ) {
  return coord_for_unit( id );
}

LUA_FN( units_from_coord, lua::table, Coord c ) {
  lua::state& st  = lua_global_state();
  lua::table  res = st.table.create();
  int         i   = 1;
  for( UnitId id : units_from_coord( c ) ) res[i++] = id;
  return res;
}

} // namespace

} // namespace rn
