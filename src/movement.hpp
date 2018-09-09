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

// The following two enums describe the possible outcomes of a
// hypothetical move of a unit from one square to another. The
// game is designed so that only one of these can be true for a
// given unit moving to a given square. Also, these are indepen-
// dent of where the unit is coming from, i.e., they are only a
// function of the target square of the move.

enum class e_unit_mv_good {
  map_to_map,
  board_ship
/*land_fall,
  high_seas,
  dock,
  attack_nation,
  attack_tribe,
  attack_privateer,
  enter_village_live,
  enter_village_scout,
  trade_with_nation,
  trade_with_village,
  enter_ruins*/
};

enum class e_unit_mv_error {
  map_edge,
  land_forbidden,
  water_forbidden,
  insufficient_movement_points,
  board_ship_full
};

using v_unit_mv_desc = std::variant<
  e_unit_mv_good,
  e_unit_mv_error
>;

// Describes what would happen if a unit were to move to a
// given square.
struct UnitMoveDesc {
  // The target square of move being described.
  Coord coords;
  // Is it flat out impossible
  bool can_move;
  // Description of what would happen if the move were carried
  // out.  This will also be set even if can_move == false.
  v_unit_mv_desc desc;
  // Cost in movement points that would be incurred; this is
  // a positive number.
  MovementPoints movement_cost;
  // Unit that is the target of an action, e.g., unit to
  // be attacked, ship to be boarded, etc.  Not relevant
  // in all contexts.
  UnitId target_unit;
};

// Called at the beginning of each turn; marks all units
// as not yet having moved.
void reset_moves();

UnitMoveDesc move_consequences( UnitId id, Coord coords );
void move_unit_to( UnitId, Coord target );

} // namespace rn
