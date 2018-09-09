/****************************************************************
* ownership.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles creation, destruction, and ownership of
*              units.
*
*****************************************************************/
#include "aliases.hpp"
#include "base-util.hpp"
#include "macros.hpp"
#include "ownership.hpp"
#include "world.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

unordered_map<UnitId, Unit> units;

enum class e_unit_ownership {
  map
};
unordered_map<UnitId, e_unit_ownership> unit_ownership;

// For units that are on (owned by) the map.
unordered_map<Coord, unordered_set<UnitId>> units_from_coords;
unordered_map<UnitId, Coord> coords_from_unit;

void disown_unit( UnitId id ) {
  ASSIGN_ASSERT_OPT( it, has_key( unit_ownership, id ) );
  switch( it->second ) {
    case e_unit_ownership::map:
      // First remove from coords_from_unit
      ASSIGN_ASSERT_OPT( coords_it, has_key( coords_from_unit, id ) );
      auto coords = coords_it->second;
      coords_from_unit.erase( coords_it );
      // Now remove from units_from_coords
      ASSIGN_ASSERT_OPT( set_it, has_key( units_from_coords, coords ) );
      units_from_coords.erase( set_it );
      break;
  };
  // Probably need to do this last so iterators don't get
  // invalidated.
  unit_ownership.erase( it );
}

} // namespace

UnitIdVec units_all( optional<e_nation> nation ) {
  vector<UnitId> res; res.reserve( units.size() );
  if( nation ) {
    for( auto const& p : units )
      if( *nation == p.second.nation() )
        res.push_back( p.first );
  } else {
    for( auto const& p : units )
      res.push_back( p.first );
  }
  return res;
}

Unit& unit_from_id( UnitId id ) {
  ASSIGN_ASSERT_OPT( res, has_key( units, id ) );
  return res->second;
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( function<void( Unit& )> func ) {
  for( auto& p : units )
    func( p.second );
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
  units_from_coords[Coord{y,x}].insert( unit.id() );
  coords_from_unit[unit.id()] = Coord{y,x};
  unit_ownership[unit.id()] = e_unit_ownership::map;
  return unit.id();
}

UnitIdVec units_from_coord( Y y, X x ) {
  auto opt_set = get_val_safe( units_from_coords, Coord{y,x} );
  if( !opt_set ) return {};
  unordered_set<UnitId> const& set = (*opt_set);
  UnitIdVec res; res.reserve( set.size() );
  for( auto id : set )
    res.push_back( id );
  return res;
}

UnitIdVec units_int_rect( Rect const& rect ) {
  UnitIdVec res;
  for( Y i = rect.y; i < rect.y+rect.h; ++i )
    for( X j = rect.x; j < rect.x+rect.w; ++j )
      for( auto id : units_from_coord( i, j ) )
        res.push_back( id );
  return res;
}

OptCoord coords_for_unit_safe( UnitId id ) {
  return get_val_safe( coords_from_unit, id );
}

Coord coords_for_unit( UnitId id ) {
  auto opt_coord = coords_for_unit_safe( id );
  ASSERT( opt_coord );
  return *opt_coord;
}

void ownership_change_to_map( UnitId id, Coord target ) {
  disown_unit( id );
  // Add unit to new square.
  units_from_coords[{target.y,target.x}].insert( id );
  // Set unit coords to new value.
  coords_from_unit[id] = {target.y,target.x};
  unit_ownership[id] = e_unit_ownership::map;
}

} // namespace rn
