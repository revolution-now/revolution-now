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

// Revolution Now
#include "analysis.hpp"
#include "unit.hpp"

namespace rn {

// The following two enums describe the possible categories of a
// hypothetical move of a unit from one square to another in a
// world where there are no foreign units and where movement
// points are infinite. The game is designed so that only one of
// these can be true for a given unit moving to a given square.
//
// If the result of a move is one of the e_unit_travel_error then
// that means the move is not possible regardless of what
// occupies the target square. If, on the other hand, the result
// of the move is one of e_unit_travel_good, then that means
// that:
//
//   a) If there are no foreign entities in the target square
//      then the move is possible provided that the unit has
//      enough movement points (which has to be checked
//      separately).
//
//   b) If there are foreign entities in the square then this
//      move may be possible, but must be processed elsewhere.
//
// This module is only concerned with checking if a move is phys-
// ically possible without regard to what entities occupy the
// target square and without regard to movement points. The
// reason that we don't care about movement points here is that
// the question as to whether the move is allowed on the basis of
// movement points depends on what units occupy the target
// square. E.g., if a unit only has 1/3 movement points and wants
// to move to a square that requires 1 movement point then they
// cannot do so if there are no foreign units in the target
// square, but they can do so if they are a military unit and
// there are foreign units in the square (in which case they will
// take an attack penalty).

enum class ND e_unit_travel_good {
  map_to_map,
  board_ship,
  offboard_ship,
  land_fall,
  /*clang-format off
  high_seas,
  dock,
  enter_ruins
  clang-format on*/
};

enum class ND e_unit_travel_error {
  map_edge,
  land_forbidden,
  water_forbidden,
  board_ship_full,
};

using unit_travel_verdict =
    std::variant<e_unit_travel_good, e_unit_travel_error>;

struct TravelAnalysis : public OrdersAnalysis<TravelAnalysis> {
  TravelAnalysis( UnitId id_, Orders orders_ )
    : parent_t( id_, orders_ ) {}

  // ------------------------ Data -----------------------------

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
  unit_travel_verdict desc{};

  // Cost in movement points that would be incurred; this is
  // a positive number.
  MovementPoints movement_cost{};

  // Unit that is the target of an action, e.g., ship to be
  // boarded, etc. Not relevant in all contexts.
  Opt<UnitId> target_unit{};

  // ---------------- "Virtual" Methods ------------------------

  bool allowed_impl() const;
  bool confirm_explain_impl() const;
  void affect_orders_impl() const;

  static Opt<TravelAnalysis> analyze_impl( UnitId id,
                                           Orders orders );
};

} // namespace rn
