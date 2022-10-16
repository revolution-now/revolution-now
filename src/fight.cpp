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
#include "irand.hpp"
#include "logger.hpp"

// ss
#include "ss/unit.hpp"

// config
#include "config/unit-type.hpp"

namespace rn {

namespace {} // namespace

// TODO: see doc/col1-fighting.txt for links that explain how the
// original game computes the bonuses.
//
// TODO: See NAMES.TXT for the stats on the unit types and check
// them against those in our config file.
//
// TODO: need to take into account Francis Drake who increases
// the strength of privateers by 50%.
//
// TODO: need to take into account George Washington here who
// makes winners upgrade with 100% probability.
FightStatistics fight_statistics( IRand&      rand,
                                  Unit const& attacker,
                                  Unit const& defender ) {
  auto attack_points = attacker.desc().attack_points;
  auto defend_points = defender.desc().defense_points;
  auto winning_probability =
      double( attack_points ) /
      double( attack_points + defend_points );
  CHECK( attack_points > 0 );
  CHECK( winning_probability <= 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return { rand.bernoulli( winning_probability ) };
}

} // namespace rn
