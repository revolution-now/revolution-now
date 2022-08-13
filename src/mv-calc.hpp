/****************************************************************
**mv-calc.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-28.
*
* Description: Determines if/how a unit can make a move requiring
*              a certain number of movement points.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "mv-points.hpp"

namespace rn {

struct TS;
struct Unit;

/****************************************************************
** MovementPointsAnalysis
*****************************************************************/
// Although a unit has a fixed number of movement points, there
// are exceptions made to this rule probably for a good player
// experience. In particular, in some cases the unit is allowed
// to use more points than it has.
//
//   1. A unit that hasn't yet moved at all needs to be able to
//      move onto any square even if they don't have enough move-
//      ment points. If this weren't the case then e.g. a free
//      colonist would never be able to move along most of the
//      map, since e.g. mountains and forest require more than
//      one movement point.
//   2. The original game will allow a unit to exceed their total
//      movement points with some degree of randomness. In par-
//      ticular, when a unit with N>0 movement points left at-
//      tempts to move onto a square requiring M>N movement
//      points (i.e., they don't have enough) then the unit will
//      be able to succeed in moving with probability N/M. For
//      example, if a free colonist moves one square along a
//      road, they will have 2/3 movement points left. At this
//      point, the game will allow them to move onto a square
//      that requires two movement points, but only with a proba-
//      bility of (2/3)/2=1/3.
//
struct [[nodiscard]] MovementPointsAnalysis {
  // This function should be called to determine whether the
  // analysis resulted in an allowed move.
  bool allowed() const;

  // How many points should actually be deducted from this unit
  // after making (or attempting to make) the move? This will en-
  // sure that the unit's points don't go below zero. Note that
  // this can be called even if the move is not allowed, since in
  // that case the unit forfeits its movement points.
  MovementPoints points_to_subtract() const;

  // Number of movement points the unit has available, not in-
  // cluding any overdraw allowance awarded.
  MovementPoints has = {};

  // Number of movement points theoretically needed. This is the
  // value specified as an input.
  MovementPoints needed = {};

  // Has the unit not moved yet; if so, they will always be al-
  // lowed to move.
  bool using_start_of_turn_exemption = {};

  // If this is true then 1) the unit did not have the start of
  // turn exemption, and 2) it also didn't have enough movement
  // points, and so some randomness was used to determine if the
  // move would proceed anyway, and the result was in the affir-
  // mative.
  bool using_overdraw_allowance = {};

  bool operator==( MovementPointsAnalysis const& ) const =
      default;
};

/****************************************************************
** Public API
*****************************************************************/
// Given a unit and an amount of movement points, this function
// will determine whether the unit can move, possibly including
// some randomness (hence repeated identical calls to it can in
// general return different results). The analysis is based on
// the number of movement points that the unit has remaining as
// well as how many are needed.
//
// Note that the movement points passed into this function must
// be the final points required after considering e.g. colonies,
// roads, rivers, or ships on the square.
MovementPointsAnalysis can_unit_move_based_on_mv_points(
    TS& ts, Unit const& unit, MovementPoints needed );

} // namespace rn
