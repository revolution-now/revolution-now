/****************************************************************
**fight.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-14.
*
* Description: Handles statistics for individual combats.
*
*****************************************************************/
#include "fight.hpp"

// Revolution Now
#include "rand.hpp"

namespace rn {

namespace {} // namespace

FightStatistics fight_statistics( UnitId attacker,
                                  UnitId defender ) {
  (void)attacker;
  (void)defender;
  return {rng::flip_coin()};
}

} // namespace rn
