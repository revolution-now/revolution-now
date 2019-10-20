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
#include "errors.hpp"
#include "fb.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "terrain.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/keyval.hpp"
#include "base-util/variant.hpp"

// Flatbuffers
#include "fb/sg-unit_generated.h"

// C++ standard library
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Unit ) {
  // This will be called after deserialization.
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  using UnitStorageMap_t = unordered_map<UnitId, Unit>;

  // clang-format off
  SAVEGAME_MEMBERS( Unit,
  ( UnitStorageMap_t,    units          ));
  // clang-format on
};
SAVEGAME_IMPL( Unit );

// Holds deleted units for debugging purposes (they will never be
// resurrected and their IDs will never be reused).
FlatSet<UnitId> g_deleted_units;

// FIXME: get rid of all these separate maps and represent owner-
//        ship as a single map from UnitId --> ADT.

// For units that are on (owned by) the world (map).
unordered_map<Coord, unordered_set<UnitId>> units_from_coords;
unordered_map<UnitId, Coord>                coords_from_unit;

// For units that are held as cargo.
unordered_map</*held*/ UnitId, /*holder*/ UnitId>
    holder_from_held;

// For units that are owned by either the high seas or by the eu-
// rope view (NOTE: this does NOT refer to the King's army; "own-
// ership" here refers to where the unit is located in the game).
unordered_map<UnitId, UnitEuroPortViewState_t>
    g_euro_port_view_units;

enum class e_unit_ownership {
  // Unit is on the map.  This includes units that are stationed
  // in colonies.  It does not include units in indian villages
  // or in boats.
  world,
  // This includes units in boats or wagons.
  cargo,
  // This is for units that are on the high seas, in port, or on
  // dock in the europe view.
  old_world
};

unordered_map<UnitId, e_unit_ownership> unit_ownership;

void check_europort_state_invariants(
    UnitEuroPortViewState_t const& info ) {
  switch_( info ) {
    case_( UnitEuroPortViewState::outbound, percent ) {
      CHECK( percent >= 0.0 );
      CHECK( percent < 1.0 );
    }
    case_( UnitEuroPortViewState::inbound, percent ) {
      CHECK( percent >= 0.0 );
      CHECK( percent < 1.0 );
    }
    case_( UnitEuroPortViewState::in_port ) {
      //
    }
    switch_exhaustive;
  }
}

} // namespace

string debug_string( UnitId id ) {
  return debug_string( unit_from_id( id ) );
}

Vec<UnitId> units_all( optional<e_nation> nation ) {
  vector<UnitId> res;
  res.reserve( SG().units_.size() );
  if( nation ) {
    for( auto const& p : SG().units_ )
      if( *nation == p.second.nation() )
        res.push_back( p.first );
  } else {
    for( auto const& p : SG().units_ ) res.push_back( p.first );
  }
  return res;
}

bool unit_exists( UnitId id ) {
  bool exists  = bu::has_key( SG().units_, id ).has_value();
  bool deleted = bu::has_key( g_deleted_units, id ).has_value();
  if( exists )
    CHECK( !deleted, "{}: exists: {}, deleted: {}.",
           /*no debug_string*/ id, exists, deleted );
  return exists;
}

Unit& unit_from_id( UnitId id ) {
  CHECK( unit_exists( id ) );
  return val_or_die( SG().units_, id );
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( tl::function_ref<void( Unit& )> func ) {
  for( auto& p : SG().units_ ) func( p.second );
}

// Should not be holding any references to the unit after this.
void destroy_unit( UnitId id ) {
  CHECK( unit_exists( id ) );
  CHECK( !g_deleted_units.contains( id ) );
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
  auto it = SG().units_.find( id );
  CHECK( it != SG().units_.end() );
  SG().units_.erase( it->first );
  g_deleted_units.insert( id );
}

Unit& create_unit( e_nation nation, e_unit_type type ) {
  Unit unit( nation, type );
  auto id = unit.id_;
  CHECK( !bu::has_key( SG().units_, id ) );
  CHECK( !g_deleted_units.contains( id ) );
  // To avoid requirement of operator[] that we have a default
  // constructor on Unit.
  SG().units_.emplace( id, move( unit ) );
  return SG().units_.find( id )->second;
}

/****************************************************************
** Map Ownership
*****************************************************************/
unordered_set<UnitId> const& units_from_coord( Y y, X x ) {
  static unordered_set<UnitId> empty = {};
  CHECK( square_exists( y, x ) );
  auto opt_set = bu::val_safe( units_from_coords, Coord{y, x} );
  return opt_set.value_or( empty );
}

unordered_set<UnitId> const& units_from_coord( Coord c ) {
  return units_from_coord( c.y, c.x );
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

Opt<e_nation> nation_from_coord( Coord coord ) {
  auto const& units = units_from_coord( coord );
  if( units.empty() ) return nullopt;
  e_nation first = unit_from_id( *units.begin() ).nation();
  for( auto const& id : units )
    CHECK( first == unit_from_id( id ).nation() );
  return first;
}

void move_unit_from_map_to_map( UnitId id, Coord dest ) {
  CHECK( unit_ownership[id] == e_unit_ownership::world );
  ustate_change_to_map( id, dest );
}

Vec<UnitId> units_in_rect( Rect const& rect ) {
  Vec<UnitId> res;
  for( Y i = rect.y; i < rect.y + rect.h; ++i )
    for( X j = rect.x; j < rect.x + rect.w; ++j )
      for( auto id : units_from_coord( i, j ) )
        res.push_back( id );
  return res;
}

Coord coords_for_unit( UnitId id ) {
  ASSIGN_CHECK_OPT( res, coords_for_unit_safe( id ) );
  return res;
}

// If this function makes recursive calls it should always call
// the _safe variant since this function should not throw.
Opt<Coord> coords_for_unit_safe( UnitId id ) {
  ASSIGN_OR_RETURN( ownership,
                    bu::val_safe( unit_ownership, id ) );
  switch( ownership ) {
    case e_unit_ownership::world:
      return bu::val_safe( coords_from_unit, id );
    case e_unit_ownership::cargo: {
      ASSIGN_OR_RETURN( holder,
                        bu::val_safe( holder_from_held, id ) );
      // Coordinates of unit are coordinates of holder.
      return coords_for_unit_safe( holder );
    }
    case e_unit_ownership::old_world: //
      return nullopt;
  };
  UNREACHABLE_LOCATION;
}

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nullopt otherwise.
Opt<UnitId> is_unit_onboard( UnitId id ) {
  auto opt_iter = bu::has_key( holder_from_held, id );
  return opt_iter ? optional<UnitId>( ( **opt_iter ).second )
                  : nullopt;
}

/****************************************************************
** EuroPort View Ownership
*****************************************************************/
Opt<Ref<UnitEuroPortViewState_t>> unit_euro_port_view_info(
    UnitId id ) {
  ASSIGN_OR_RETURN( it,
                    bu::has_key( g_euro_port_view_units, id ) );
  return it->second;
}

Vec<UnitId> units_in_euro_port_view() {
  Vec<UnitId> res;
  for( auto const& p : g_euro_port_view_units )
    res.push_back( p.first );
  return res;
}

/****************************************************************
** For Testing / Development Only
*****************************************************************/
UnitId create_unit_on_map( e_nation nation, e_unit_type type,
                           Y y, X x ) {
  Unit& unit = create_unit( nation, type );
  ustate_change_to_map( unit.id(), {x, y} );
  return unit.id();
}

UnitId create_unit_in_euroview_port( e_nation    nation,
                                     e_unit_type type ) {
  Unit& unit = create_unit( nation, type );
  ustate_change_to_euro_port_view(
      unit.id(), UnitEuroPortViewState::in_port{} );
  return unit.id();
}

UnitId create_unit_as_cargo( e_nation nation, e_unit_type type,
                             UnitId holder ) {
  Unit& unit = create_unit( nation, type );
  ustate_change_to_cargo( holder, unit.id() );
  return unit.id();
}

/****************************************************************
** Low-Level Ownership Change Functions
*****************************************************************/
void ustate_change_to_map( UnitId id, Coord const& target ) {
  internal::ustate_disown_unit( id );
  // Add unit to new square.
  units_from_coords[{target.y, target.x}].insert( id );
  // Set unit coords to new value.
  coords_from_unit[id] = {target.y, target.x};
  unit_ownership[id]   = e_unit_ownership::world;
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
  CHECK( cargo_hold.try_add( Cargo{held}, slot ) );
  unit_from_id( held ).sentry();
  // Set new ownership
  unit_ownership[held]   = e_unit_ownership::cargo;
  holder_from_held[held] = new_holder;
}

void ustate_change_to_cargo( UnitId new_holder, UnitId held ) {
  auto& cargo = unit_from_id( new_holder ).cargo();
  for( int i = 0; i < cargo.slots_total(); ++i ) {
    if( cargo.fits( held, i ) ) {
      ustate_change_to_cargo( new_holder, held, i );
      return;
    }
  }
  FATAL( "Unit {} cannot be placed in unit {}'s cargo: {}",
         debug_string( held ), debug_string( new_holder ),
         cargo );
}

void ustate_change_to_euro_port_view(
    UnitId id, UnitEuroPortViewState_t info ) {
  check_europort_state_invariants( info );
  if( !bu::has_key( g_euro_port_view_units, id ) ) {
    internal::ustate_disown_unit( id );
    unit_ownership[id] = e_unit_ownership::old_world;
  }
  g_euro_port_view_units[id] = info;
}

/****************************************************************
** Testing
*****************************************************************/
namespace testing_only {

void reset_unit_creation() {
  SG().units_.clear();
  g_deleted_units.clear();
  units_from_coords.clear();
  coords_from_unit.clear();
  holder_from_held.clear();
  g_euro_port_view_units.clear();
  unit_ownership.clear();

  reset_unit_ids();
}

} // namespace testing_only

/****************************************************************
** Do not call directly
*****************************************************************/
namespace internal {
// The purpose of this function is *only* to manipulate the above
// global maps. It does not follow any of the associated proce-
// dures that need to be followed when a unit is added, removed,
// or moved from one map to another (or from one owner to anoth-
// er).
//
// Specifically, it will erase any ownership that is had over the
// given unit and mark it as unowned.
void ustate_disown_unit( UnitId id ) {
  // If there is no ownership then return and do nothing.
  ASSIGN_OR_RETURN_( it, bu::has_key( unit_ownership, id ) );
  switch( it->second ) {
    // For some strange reason we need braces around this case
    // statement otherwise we get errors... something to do with
    // local variables declared inside of it.
    case e_unit_ownership::world: {
      // First remove from coords_from_unit
      ASSIGN_CHECK_OPT( pair_it,
                        bu::has_key( coords_from_unit, id ) );
      auto coords = pair_it->second;
      coords_from_unit.erase( pair_it );
      // Now remove from units_from_coords
      ASSIGN_CHECK_OPT(
          set_it, bu::has_key( units_from_coords, coords ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() ) units_from_coords.erase( set_it );
      break;
    }
    case e_unit_ownership::cargo: {
      ASSIGN_CHECK_OPT( pair_it,
                        bu::has_key( holder_from_held, id ) );
      auto& holder_unit = unit_from_id( pair_it->second );
      ASSIGN_CHECK_OPT( slot_idx,
                        holder_unit.cargo().find_unit( id ) );
      holder_unit.cargo().remove( slot_idx );
      holder_from_held.erase( pair_it );
      break;
    }
    case e_unit_ownership::old_world: {
      CHECK( bu::has_key( g_euro_port_view_units, id ) );
      // Ensure the unit has no cargo.
      CHECK( unit_from_id( id )
                 .cargo()
                 .count_items_of_type<UnitId>() == 0 );
      g_euro_port_view_units.erase( id );
      break;
    }
  };
  // Probably need to do this last so iterators don't get
  // invalidated.
  unit_ownership.erase( it );
}
} // namespace internal

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_on_map, Unit const&, e_nation nation,
        e_unit_type type, Coord const& coord ) {
  auto id = create_unit_on_map( nation, type, coord.y, coord.x );
  lg.info( "created a {} on square {}.", unit_desc( type ).name,
           coord );
  return unit_from_id( id );
}

LUA_FN( unit_from_id, Unit const&, UnitId id ) {
  return unit_from_id( id );
}

} // namespace

} // namespace rn
