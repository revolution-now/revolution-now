/****************************************************************
**fight-stats.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-14.
*
* Description: Handles statistics for individual combats.
*
*****************************************************************/
#include "fight-stats.hpp"

// Revolution Now
#include "irand.hpp"
#include "logger.hpp"
#include "ts.hpp"

// config
#include "config/natives.hpp"

// ss
#include "ss/native-unit.rds.hpp"
#include "ss/unit.hpp"

// config
#include "config/unit-type.hpp"

namespace rn {

namespace {

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
//
// TODO: dwelling type will affect defense points.

} // namespace

FightStatsEuroAttackEuro fight_stats_euro_attack_euro(
    TS& ts, Unit const& attacker, Unit const& defender ) {
  int const attack_points  = attacker.desc().attack_points;
  int const defense_points = defender.desc().defense_points;
  auto      winning_probability =
      double( attack_points ) /
      double( attack_points + defense_points );
  CHECK( attack_points > 0 );
  CHECK( winning_probability <= 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return { ts.rand.bernoulli( winning_probability ) };
}

FightStatsEuroAttackBrave fight_stats_euro_attack_brave(
    TS& ts, Unit const& attacker, NativeUnit const& defender ) {
  int const attack_points  = attacker.desc().attack_points;
  int const defense_points = unit_attr( defender.type ).combat;
  auto      winning_probability =
      double( attack_points ) /
      double( attack_points + defense_points );
  CHECK( attack_points > 0 );
  CHECK( winning_probability <= 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return { ts.rand.bernoulli( winning_probability ) };
}

FightStatsEuroAttackDwelling fight_stats_euro_attack_dwelling(
    TS& ts, Unit const& attacker, Dwelling const& ) {
  int const attack_points = attacker.desc().attack_points;
  int const defense_points =
      unit_attr( e_native_unit_type::brave ).combat;
  // TODO: dwelling type will affect defense points.
  auto winning_probability =
      double( attack_points ) /
      double( attack_points + defense_points );
  CHECK( attack_points > 0 );
  CHECK( winning_probability <= 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return { ts.rand.bernoulli( winning_probability ) };
}

FightStatsEuroAttackEuro
make_fight_stats_for_attacking_ship_on_land() {
  return {
      .attacker_wins = true,
  };
}

} // namespace rn
