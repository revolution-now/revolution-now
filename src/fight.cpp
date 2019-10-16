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
#include "logging.hpp"
#include "rand.hpp"
#include "ustate.hpp"

namespace rn {

namespace {} // namespace

FightStatistics fight_statistics( UnitId attacker_id,
                                  UnitId defender_id ) {
  auto const& attacker      = unit_from_id( attacker_id );
  auto const& defender      = unit_from_id( defender_id );
  auto        attack_points = attacker.desc().attack_points;
  auto        defend_points = defender.desc().defense_points;
  auto        winning_probability =
      double( attack_points ) /
      double( attack_points + defend_points );
  CHECK( attack_points > 0 );
  CHECK( winning_probability <= 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return {rng::flip_coin( winning_probability )};
}

} // namespace rn
