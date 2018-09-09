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

#include "base-util.hpp"
#include "id.hpp"
#include "macros.hpp"
#include "ownership.hpp"
#include "world.hpp"

using namespace std;

namespace rn {

namespace {

} // namespace

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

  if( unit.desc().boat && square.land ) {
    return {{y, x}, false, e_unit_mv_desc::land_forbidden, cost};
  }
  if( !unit.desc().boat && !square.land ) {
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

  // It is safe to move, so physically move.
  ownership_change_to_map( id, target );

  unit.consume_mv_points( move_desc.movement_cost );
}

} // namespace rn
