/****************************************************************
**mv-calc.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-28.
*
* Description: Calculation of movement points needed for a move.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "map-square.hpp"
#include "mv-points.hpp"
#include "unit.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

/****************************************************************
** MovementPointsAnalyzis
*****************************************************************/
// Although a unit has a fixed number of movement points, there
// are exceptions made to this rule probably for a good player
// experience. In particular, in some cases the unit is allowed
// to use more points than it has.
//
//   1. A unit that hasn't yet moved at all needs to be able to
//      move onto any square even if they don't have enough move-
//      ment points. If this weren't the case then e.g. a free
//      colonist would never be able to move onto mountains,
//      which normally requires three movement points.
//   2. It appears that the original game will allow a unit to
//      exceed their total movement points so long as they don't
//      go more than 1/3 below zero. For example, if a free
//      colonist moves one square along a road, they will have
//      2/3 movement points left. At this point, the game will
//      allow them to move onto a square that requires one move-
//      ment point, but not more than that. E.g., if a free
//      colonist has 2/3 movement points they cannot move onto a
//      forrest (2) or mountains (3), but they can move onto a
//      grassland. Likewise, if a free colonist has 1/3 movement
//      points they will not be allowed to move onto grassland
//      either (which requires one point) since that would put
//      them at 2/3 in the red.
//
struct [[nodiscard]] MovementPointsAnalysis {
  // This function should be called to determine whether the
  // above numbers allow for the move.
  bool allowed() const;

  // Is the unit relying on the start-of-turn exemption to make
  // this move?
  bool using_start_of_turn_exemption() const;

  // Is the unit relying on the overdraw allowance to overdraw
  // their movement points for this move?
  bool using_overdraw_allowance() const;

  // How many points should actually be deducted from this unit
  // after making the move? This will ensure that the unit's
  // points don't go below zero. Only call this if the move is
  // allowed.
  MovementPoints points_to_subtract() const;

  // Maximum of deficit the unit is allowed to take on when
  // making a move. In the original game this seems to always be
  // 1/3, though no official references on this were found. This
  // is a positive number.
  inline static constexpr MovementPoints kOverdrawAllowance =
      MovementPoints::_1_3();

  // Number of movement points the unit has available, not in-
  // cluding the overdraw allowance.
  MovementPoints has = {};

  // Number of movement points theoretically needed. This will be
  // determined by the terrain in the target square as well as
  // whether there is a road or river connecting the two squares,
  // as well as the direction from the src tile to the dst tile.
  MovementPoints needs = {};

  // Has the unit not moved yet; if so, they will be allowed to
  // move onto any square.
  bool has_start_of_turn_exemption = {};

  bool operator==( MovementPointsAnalysis const& ) const =
      default;
};

static_assert( MovementPointsAnalysis::kOverdrawAllowance > 0 );

/****************************************************************
** Public API
*****************************************************************/
// The direction is from src to dst.
MovementPointsAnalysis expense_movement_points(
    Unit const& unit, MapSquare const& src_square,
    MapSquare const& dst_square, e_direction d );

} // namespace rn
