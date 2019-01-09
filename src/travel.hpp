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
// points are ignored. The game is designed so that only one of
// these can be true for a given unit moving to a given square.
//
// If the result of a move is one of the e_unit_travel_error then
// that means the move is not possible regardless of what occu-
// pies the target square. If the result of the move is one of
// e_unit_travel_good then that means that the move would be pos-
// sible assuming enough movement points (and, again, assuming no
// foreign entities in the target square).

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

  // Unit that is the target of an action, e.g., ship to be
  // boarded, etc. Not relevant in all contexts.
  Opt<UnitId> target_unit{};

  // ---------------- "Virtual" Methods ------------------------

  bool allowed_() const;
  bool confirm_explain_() const;
  void affect_orders_() const;

  static Opt<TravelAnalysis> analyze_( UnitId id,
                                       Orders orders );
};

} // namespace rn
