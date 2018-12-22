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
#include "aliases.hpp"
#include "errors.hpp"
#include "util.hpp"
#include "world.hpp"

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

enum class e_unit_ownership {
  // Unit is on the map.  This includes units that are stationed
  // in colonies.  It does not include units in indian villages
  // or in boats.
  world,
  // This includes units in boats or wagons.
  cargo
};

unordered_map<UnitId, e_unit_ownership> unit_ownership;

} // namespace

// The purpose of this function is *only* to manipulate the above
// global maps. It does not follow any of the associated proce-
// dures that need to be followed when a unit is added, removed,
// or moved from one map to another.
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
    case e_unit_ownership::cargo:
      ASSIGN_CHECK_OPT( pair_it,
                        has_key( holder_from_held, id ) );
      auto& holder_unit = unit_from_id( pair_it->second );
      holder_unit.cargo().remove( id );
      holder_from_held.erase( pair_it );
      break;
  };
  // Probably need to do this last so iterators don't get
  // invalidated.
  unit_ownership.erase( it );
}

UnitIdVec units_all( optional<e_nation> nation ) {
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

Unit& unit_from_id( UnitId id ) {
  ASSIGN_CHECK_OPT( res, has_key( units, id ) );
  return res->second;
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( function<void( Unit& )> const& func ) {
  for( auto& p : units ) func( p.second );
}

Unit& create_unit( e_nation nation, e_unit_type type ) {
  Unit unit( nation, type );
  auto id = unit.id_;
  // To avoid requirement of operator[] that we have a default
  // constructor on Unit.
  units.emplace( id, move( unit ) );
  return units.find( id )->second;
}

// need to think about what this API should be.
UnitId create_unit_on_map( e_unit_type type, Y y, X x ) {
  Unit& unit = create_unit( e_nation::dutch, type );
  units_from_coords[Coord{y, x}].insert( unit.id() );
  coords_from_unit[unit.id()] = Coord{y, x};
  unit_ownership[unit.id()]   = e_unit_ownership::world;
  return unit.id();
}

unordered_set<UnitId> const& units_from_coord( Y y, X x ) {
  static unordered_set<UnitId> empty = {};
  auto opt_set = val_safe( units_from_coords, Coord{y, x} );
  return opt_set.value_or( empty );
}

UnitIdVec units_int_rect( Rect const& rect ) {
  UnitIdVec res;
  for( Y i = rect.y; i < rect.y + rect.h; ++i )
    for( X j = rect.x; j < rect.x + rect.w; ++j )
      for( auto id : units_from_coord( i, j ) )
        res.push_back( id );
  return res;
}

OptCoord coords_for_unit_safe( UnitId id ) {
  return val_safe( coords_from_unit, id );
}

Coord coords_for_unit( UnitId id ) {
  ASSIGN_CHECK_OPT( it, has_key( unit_ownership, id ) );
  switch( it->second ) {
    case e_unit_ownership::world: {
      auto opt_coord = coords_for_unit_safe( id );
      CHECK( opt_coord );
      return *opt_coord;
    }
    case e_unit_ownership::cargo:
      ASSIGN_CHECK_OPT( pair_it,
                        has_key( holder_from_held, id ) );
      // Coordinates of unit are coordinates of holder.
      return coords_for_unit( pair_it->second );
  };
  DIE( "should not be here." );
  return {};
}

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
  auto& cargo_hold = unit_from_id( new_holder ).cargo();
  // We're clear (at least on our end).
  ownership_disown_unit( held );
  cargo_hold.add( held );
  // Set new ownership
  unit_ownership[held]   = e_unit_ownership::cargo;
  holder_from_held[held] = new_holder;
}

// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nullopt otherwise.
OptUnitId is_unit_onboard( UnitId id ) {
  auto opt_iter = has_key( holder_from_held, id );
  return opt_iter ? optional<UnitId>( ( **opt_iter ).second )
                  : nullopt;
}

} // namespace rn
