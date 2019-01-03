/****************************************************************
**movement.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Physical movement of units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "unit.hpp"

namespace rn {

// The following two enums describe the possible categories of a
// hypothetical move of a unit from one square to another. The
// game is designed so that only one of these can be true for a
// given unit moving to a given square.

enum class ND e_unit_mv_good {
  map_to_map,
  board_ship,
  offboard_ship,
  land_fall
  /*clang-format off
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
  clang-format on*/
};

enum class ND e_unit_mv_error {
  map_edge,
  land_forbidden,
  water_forbidden,
  insufficient_movement_points,
  board_ship_full
};

using v_unit_mv_desc =
    std::variant<e_unit_mv_good, e_unit_mv_error>;

// Describes what would happen if a unit were to move to a given
// square.
struct ND ProposedMoveAnalysisResult {
  // If this move is allowed and executed, will the unit actually
  // move to the target square as a result? Normally the answer
  // is yes, however there are cases when the answer is no, such
  // as when a ship makes landfall.
  bool unit_would_move{};
  // The square on which the unit resides.
  Coord move_src{};
  // The square toward which the move is aimed; note that if/when
  // this move is executed the unit will not necessarily move to
  // this square (it depends on the kind of move being made).
  // That said, this field will always contain a valid and mean-
  // ingful value since there must always be a move order in
  // order for this data structure to even be populated.
  Coord move_target{};
  // Description of what would happen if the move were carried
  // out. This can also serve as a binary indicator of whether
  // the move is possible by checking the type held, as the can_-
  // move() function does as a convenience.
  v_unit_mv_desc desc{};
  // Cost in movement points that would be incurred; this is
  // a positive number.
  MovementPoints movement_cost{};
  // Unit that is the target of an action, e.g., unit to
  // be attacked, ship to be boarded, etc.  Not relevant
  // in all contexts.
  Opt<UnitId> target_unit{};
  // Units that will be waiting for orders and which should be
  // prioritized in the "orders" loop after this move is made.
  // This field is only relevant for certain (valid) moves. NOTE:
  // units will be prioritized in reverse order of this vector,
  // i.e., the last unit will be up first.
  std::vector<UnitId> to_prioritize{};

  // Is it possible to move at all.  This just checks that
  // the the `desc` holds the right type.
  bool allowed() const;
};

// Called at the beginning of each turn; marks all units
// as not yet having moved.
void reset_moves();

ND ProposedMoveAnalysisResult
   analyze_proposed_move( UnitId id, e_direction d );

void move_unit( UnitId                            id,
                ProposedMoveAnalysisResult const& analysis );

// Checks that the move is possible (if not, returns false) and,
// if so, will check the type of move and determine whether the
// player needs to be asked for any kind of confirmation. In ad-
// dition, if the move is not allowed, the player may be given an
// explantation as to why.
bool confirm_explain_move(
    ProposedMoveAnalysisResult const& analysis );

} // namespace rn
