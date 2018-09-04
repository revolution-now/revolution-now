/****************************************************************
* movement.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Ownership, evolution and movement of units.
*
*****************************************************************/
#include "movement.hpp"

#include "macros.hpp"
#include "world.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

// For units that are on (owned by) the map.
unordered_map<Coord, unordered_set<UnitId>> units_from_coords;
unordered_map<UnitId, Coord> coords_from_unit;
  
#if 1
namespace explicit_types {
  // These are to make the auto-completer happy since it doesn't
  // want to recognize the fully generic templated one.
  OptCRef<unordered_set<UnitId>> get_val_safe(
        unordered_map<Coord,unordered_set<UnitId>> const& m,
        Coord const& k ) {
      auto found = m.find( k );
      if( found == m.end() )
          return std::nullopt;
      return found->second;
  }

  OptCoord get_val_safe(
        unordered_map<UnitId, Coord> const& m, UnitId k ) {
      auto found = m.find( k );
      if( found == m.end() )
          return std::nullopt;
      return found->second;
  }
}
#endif

} // namespace

// need to think about what this API should be.
UnitId create_unit_on_map( e_unit_type type, Y y, X x ) {
  Unit& unit = Unit::create( e_nation::dutch, type );
  units_from_coords[Coord{y,x}].insert( unit.id() );
  coords_from_unit[unit.id()] = Coord{y,x};
  return unit.id();
}

UnitIdVec units_from_coord( Y y, X x ) {
  auto opt_set = explicit_types::get_val_safe( units_from_coords, Coord{y,x} );
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
  return explicit_types::get_val_safe( coords_from_unit, id );
}

Coord coords_for_unit( UnitId id ) {
  auto opt_coord = coords_for_unit_safe( id );
  ASSERT( opt_coord );
  return *opt_coord;
}

// Called at the beginning of each turn; marks all units
// as not yet having moved.
void reset_moves() {
  map_units( []( Unit& unit ) {
    unit.new_turn();
  });
}

// This function will allow the move by default, and so it is the
// burden of the logic in this function to find every possible
// way that the move is *not* allowed and to flag it if that is
// the case.
UnitMoveDesc move_consequences( UnitId id, Coord coords ) {
  Y y = coords.y;
  X x = coords.x;
  MovementPoints cost( 1 );
  if( y-Y(0) >= world_size_tiles_y() ||
      x-X(0) >= world_size_tiles_x() ||
      y < 0 || x < 0 )
    return {{y, x}, false, e_unit_mv_desc::map_edge, cost};

  auto& unit = unit_from_id( id );
  auto& square = square_at( y, x );

  ASSERT( !unit.moved_this_turn() );

  if( unit.descriptor().boat && square.land ) {
    return {{y, x}, false, e_unit_mv_desc::land_forbidden, cost};
  }
  if( !unit.descriptor().boat && !square.land ) {
    return {{y, x}, false, e_unit_mv_desc::water_forbidden, cost};
  }
  if( unit.movement_points() < cost )
    return {{y, x}, false,
      e_unit_mv_desc::insufficient_movement_points, cost};
  return {{y, x}, true, e_unit_mv_desc::none, cost};
}

void move_unit_to( UnitId id, Coord target ) {
  UnitMoveDesc move_desc = move_consequences( id, target );
  // Caller should have checked this.
  ASSERT( move_desc.can_move );

  auto& unit = unit_from_id( id );
  ASSERT( !unit.moved_this_turn() );

  // Remove unit from current square.
  auto opt_current_coords = coords_for_unit_safe( id );
  // Will trigger if the unit trying to be moved is not
  // on the map.  Will eventually have to remove this.
  ASSERT( opt_current_coords );
  auto [curr_y, curr_x] = *opt_current_coords;
  auto& unit_set = units_from_coords[{curr_y,curr_x}];
  auto iter = unit_set.find( id );
  // Will trigger if an internal invariant is broken.
  ASSERT( iter != unit_set.end() );
  unit_set.erase( iter );

  // Add unit to new square.
  units_from_coords[{target.y,target.x}].insert( id );

  // Set unit coords to new value.
  coords_from_unit[id] = {target.y,target.x};

  unit.consume_mv_points( move_desc.movement_cost );
}

} // namespace rn
