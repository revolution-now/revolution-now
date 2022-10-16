/****************************************************************
**mv-calc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-28.
*
* Description: Determines if/how a unit can make a move requiring
*              a certain number of movement points.
*
*****************************************************************/
#include "mv-calc.hpp"

// Revolution Now
#include "irand.hpp"
#include "ts.hpp"

// ss
#include "ss/unit-type.hpp"
#include "ss/unit.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace rn {

/****************************************************************
** MovementPointsAnalysis
*****************************************************************/
bool MovementPointsAnalysis::allowed() const {
  if( using_start_of_turn_exemption ) return true;
  if( using_overdraw_allowance ) return true;
  return has >= needed;
}

MovementPoints MovementPointsAnalysis::points_to_subtract()
    const {
  if( needed > has ) return has;
  return needed;
}

/****************************************************************
** Public API
*****************************************************************/
MovementPointsAnalysis can_unit_move_based_on_mv_points(
    TS& ts, Player const& player, Unit const& unit,
    MovementPoints needed ) {
  MovementPoints const   has = unit.movement_points();
  MovementPointsAnalysis res{
      .has                           = has,
      .needed                        = needed,
      .using_start_of_turn_exemption = false,
      .using_overdraw_allowance      = false };
  if( has == 0 ) return res;
  if( has >= needed ) return res;
  // At this point the unit does not have enough movement points
  // to make the move, so check the exceptions.
  res.using_start_of_turn_exemption =
      ( has == movement_points( player, unit.type() ) );
  if( res.using_start_of_turn_exemption ) return res;
  CHECK_LT( has, needed );
  double const probability =
      double( has.atoms() ) / needed.atoms();
  // If this returns true then the unit gets to move anyway.
  res.using_overdraw_allowance =
      ts.rand.bernoulli( probability );
  return res;
}

} // namespace rn
