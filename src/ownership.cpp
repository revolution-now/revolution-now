/****************************************************************
**ownership.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles creation, destruction, and ownership of
*              units.
*
*****************************************************************/
#include "ownership.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "logging.hpp"
#include "terrain.hpp"
#include "util.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/variant.hpp"

// C++ standard library
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

// All units that exist anywhere.
unordered_map<UnitId, Unit> units;

// For units that are on (owned by) the world (map).
unordered_map<Coord, unordered_set<UnitId>> units_from_coords;
unordered_map<UnitId, Coord>                coords_from_unit;

// For units that are held as cargo.
unordered_map</*held*/ UnitId, /*holder*/ UnitId>
    holder_from_held;

// For units that are owned by either the high seas or by the eu-
// rope view (NOTE: this does NOT refer to the King's army; "own-
// ership" here refers to where the unit is located in the game).
unordered_map<UnitId, UnitEuroviewState_t> g_euroview_units;

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

} // namespace

string debug_string( UnitId id ) {
  return debug_string( unit_from_id( id ) );
}

// The purpose of this function is *only* to manipulate the above
// global maps. It does not follow any of the associated proce-
// dures that need to be followed when a unit is added, removed,
// or moved from one map to another (or from one owner to anoth-
// er).
//
// Specifically, it will erase any ownership that is had over the
// given unit and mark it as unowned.
void ownership_disown_unit( UnitId id ) {
  ASSIGN_CHECK_OPT( it, has_key( unit_ownership, id ) );
  switch( it->second ) {
    // For some strange reason we need braces around this case
    // statement otherwise we get errors... something to do with
    // local variables declared inside of it.
    case e_unit_ownership::world: {
      // First remove from coords_from_unit
      ASSIGN_CHECK_OPT( pair_it,
                        has_key( coords_from_unit, id ) );
      auto coords = pair_it->second;
      coords_from_unit.erase( pair_it );
      // Now remove from units_from_coords
      ASSIGN_CHECK_OPT( set_it,
                        has_key( units_from_coords, coords ) );
      auto& units_set = set_it->second;
      units_set.erase( id );
      if( units_set.empty() ) units_from_coords.erase( set_it );
      break;
    }
    case e_unit_ownership::cargo: {
      ASSIGN_CHECK_OPT( pair_it,
                        has_key( holder_from_held, id ) );
      auto& holder_unit = unit_from_id( pair_it->second );
      holder_unit.cargo().remove( id );
      holder_from_held.erase( pair_it );
      break;
    }
    case e_unit_ownership::old_world: {
      CHECK( has_key( g_euroview_units, id ) );
      // Ensure the unit has no cargo.
      CHECK( unit_from_id( id )
                 .cargo()
                 .count_items_of_type<UnitId>() == 0 );
      g_euroview_units.erase( id );
      break;
    }
  };
  // Probably need to do this last so iterators don't get
  // invalidated.
  unit_ownership.erase( it );
}

Vec<UnitId> units_all( optional<e_nation> nation ) {
  vector<UnitId> res;
  res.reserve( units.size() );
  if( nation ) {
    for( auto const& p : units )
      if( *nation == p.second.nation() )
        res.push_back( p.first );
  } else {
    for( auto const& p : units ) res.push_back( p.first );
  }
  return res;
}

bool unit_exists( UnitId id ) {
  return has_key( units, id ).has_value();
}

Unit& unit_from_id( UnitId id ) {
  ASSIGN_CHECK_OPT( res, has_key( units, id ) );
  return res->second;
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( tl::function_ref<void( Unit& )> func ) {
  for( auto& p : units ) func( p.second );
}

// Should not be holding any references to the unit after this.
void destroy_unit( UnitId id ) {
  CHECK( unit_exists( id ) );
  auto& unit = unit_from_id( id );
  // Recursively destroy any units in the cargo. We must get the
  // list of units to destroy first because we don't want to
  // destroy a cargo unit while iterating over the cargo.
  vector<UnitId> cargo_units_to_destroy;
  for( auto const& item : unit.cargo() ) {
    // Check this until we know how to deal with other types of
    // cargo.
    GET_CHECK_VARIANT( cargo_id, item, UnitId const );
    lg.info(
        "{} being destroyed as a consequence of {} being "
        "destroyed",
        debug_string( unit_from_id( cargo_id ) ),
        debug_string( unit_from_id( id ) ) );
    cargo_units_to_destroy.push_back( cargo_id );
  }
  util::map_( destroy_unit, cargo_units_to_destroy );
  ownership_disown_unit( id );
  auto it = units.find( id );
  CHECK( it != units.end() );
  units.erase( it->first );
}

Unit& create_unit( e_nation nation, e_unit_type type ) {
  Unit unit( nation, type );
  auto id = unit.id_;
  // To avoid requirement of operator[] that we have a default
  // constructor on Unit.
  units.emplace( id, move( unit ) );
  return units.find( id )->second;
}

/****************************************************************
** Map Ownership
*****************************************************************/
// need to think about what this API should be.
UnitId create_unit_on_map( e_nation nation, e_unit_type type,
                           Y y, X x ) {
  Unit& unit = create_unit( nation, type );
  units_from_coords[Coord{y, x}].insert( unit.id() );
  coords_from_unit[unit.id()] = Coord{y, x};
  unit_ownership[unit.id()]   = e_unit_ownership::world;
  return unit.id();
}

unordered_set<UnitId> const& units_from_coord( Y y, X x ) {
  static unordered_set<UnitId> empty = {};
  CHECK( square_exists( y, x ) );
  auto opt_set = val_safe( units_from_coords, Coord{y, x} );
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
  ownership_change_to_map( id, dest );
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
  ASSIGN_OR_RETURN( ownership, val_safe( unit_ownership, id ) );
  switch( ownership ) {
    case e_unit_ownership::world:
      return val_safe( coords_from_unit, id );
    case e_unit_ownership::cargo: {
      ASSIGN_OR_RETURN( holder,
                        val_safe( holder_from_held, id ) );
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
  auto opt_iter = has_key( holder_from_held, id );
  return opt_iter ? optional<UnitId>( ( **opt_iter ).second )
                  : nullopt;
}

/****************************************************************
** Euroview Ownership
*****************************************************************/
Opt<Ref<UnitEuroviewState_t>> unit_euroview_info( UnitId id ) {
  ASSIGN_OR_RETURN( it, has_key( g_euroview_units, id ) );
  return it->second;
}

FlatSet<UnitId> units_in_euroview() {
  FlatSet<UnitId> res;
  for( auto const& p : g_euroview_units ) res.insert( p.first );
  return res;
}

/****************************************************************
** Low-Level Ownership Change Functions
*****************************************************************/
void ownership_change_to_map( UnitId id, Coord const& target ) {
  ownership_disown_unit( id );
  // Add unit to new square.
  units_from_coords[{target.y, target.x}].insert( id );
  // Set unit coords to new value.
  coords_from_unit[id] = {target.y, target.x};
  unit_ownership[id]   = e_unit_ownership::world;
}

void ownership_change_to_cargo( UnitId new_holder,
                                UnitId held ) {
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
  ownership_disown_unit( held );
  cargo_hold.add( held );
  // Set new ownership
  unit_ownership[held]   = e_unit_ownership::cargo;
  holder_from_held[held] = new_holder;
}

void ownership_change_to_euroview( UnitId              id,
                                   UnitEuroviewState_t info ) {
  CHECK( !has_key( g_euroview_units, id ) );
  ownership_disown_unit( id );
  unit_ownership[id]   = e_unit_ownership::old_world;
  g_euroview_units[id] = info;
}

} // namespace rn
