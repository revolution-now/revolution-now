/****************************************************************
**mv-calc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-28.
*
* Description: Calculation of movement points needed for a move.
*
*****************************************************************/
#include "mv-calc.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace rn {

/****************************************************************
** MovementPointsAnalyzis
*****************************************************************/
bool MovementPointsAnalysis::allowed() const {
  if( has_start_of_turn_exemption ) return true;
  if( has + kOverdrawAllowance < needs ) return false;
  return true;
}

bool MovementPointsAnalysis::using_start_of_turn_exemption()
    const {
  if( has_start_of_turn_exemption )
    return ( has + kOverdrawAllowance < needs );
  return false;
}

bool MovementPointsAnalysis::using_overdraw_allowance() const {
  if( allowed() && !using_start_of_turn_exemption() &&
      needs > has ) {
    CHECK_LE( needs - has, kOverdrawAllowance );
    return true;
  }
  return false;
}

MovementPoints MovementPointsAnalysis::points_to_subtract()
    const {
  CHECK( allowed() );
  if( needs > has ) return has;
  return needs;
}

/****************************************************************
** Public API
*****************************************************************/
MovementPointsAnalysis expense_movement_points(
    Unit const& unit, MapSquare const& src_square,
    MapSquare const& dst_square, e_direction d ) {
  MovementPoints const has = unit.movement_points();
  // TODO: if there is a colony on the square then the unit can
  // probably always move into it (but might be worth checking
  // this).
  MovementPoints const needs =
      movement_points_required( src_square, dst_square, d );
  bool const has_start_of_turn_exemption =
      ( unit.movement_points() == unit.desc().movement_points );
  return MovementPointsAnalysis{
      .has   = has,
      .needs = needs,
      .has_start_of_turn_exemption =
          has_start_of_turn_exemption };
}

} // namespace rn
