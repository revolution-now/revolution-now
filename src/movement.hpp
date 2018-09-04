/****************************************************************
* movement.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Ownership, evolution and movement of units.
*
*****************************************************************/
#pragma once

#include "unit.hpp"

namespace rn {

// Game is designed so that only one of these can be true
// for a given unit moving to a given square.  Also, these
// are independent of where the unit is coming from, i.e.,
// they are only a function of the target square of the
// move.
enum class e_unit_mv_desc {
  none,
  map_edge,
  land_forbidden,
  water_forbidden,
  insufficient_movement_points,
/*land_fall,
  board_ship,
  board_ship_full
  high_seas,
  dock,
  attack_nation,
  attack_tribe,
  attack_privateer,
  enter_village_live,
  enter_village_scout,
  trade_with_nation,
  trade_with_village,
  enter_ruins
 */
};

// Describes what would happen if a unit were to move to a
// given square.
struct UnitMoveDesc {
  // The target square of move being described.
  Coord coords;
  // Is it flat out impossible
  bool can_move;
  // Description of what would happen if the move were carried
  // out.  This will also be set even if can_move == false.
  e_unit_mv_desc desc;
  // Cost in movement points that would be incurred; this is
  // a positive number.
  MovementPoints movement_cost;
};

// Not safe, probably temporary.
UnitId create_unit_on_map( e_unit_type type, Y y, X x );

// Functions for mapping between units and coordinates on the
// map.
UnitIdVec units_from_coord( Y y, X x );
UnitIdVec units_int_rect( Rect const& rect );
Coord coords_for_unit( UnitId id );

// Called at the beginning of each turn; marks all units
// as not yet having moved.
void reset_moves();

UnitMoveDesc move_consequences( UnitId id, Coord coords );
void move_unit_to( UnitId, Coord target );

} // namespace rn
