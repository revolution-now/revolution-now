/****************************************************************
**combat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-14.
*
* Description: Handles statistics for individual combats.
*
*****************************************************************/
#include "combat.hpp"

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

e_combat_winner choose_winner( IRand& rand,
                               double attacker_weight,
                               double defender_weight ) {
  CHECK_GE( attacker_weight, 0 );
  CHECK_GE( defender_weight, 0 );
  if( attacker_weight == 0 && defender_weight == 0 )
    return e_combat_winner::attacker;
  auto winning_probability =
      double( attacker_weight ) /
      double( attacker_weight + defender_weight );
  CHECK_LE( winning_probability, 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return rand.bernoulli( winning_probability )
             ? e_combat_winner::attacker
             : e_combat_winner::defender;
}

} // namespace

CombatEuroAttackEuro combat_euro_attack_euro(
    TS& ts, Unit const& attacker, Unit const& defender ) {
  int const attack_points  = attacker.desc().combat;
  int const defense_points = defender.desc().combat;
  return CombatEuroAttackEuro{
      .winner = choose_winner( ts.rand, attack_points,
                               defense_points ) };
}

CombatEuroAttackBrave combat_euro_attack_brave(
    TS& ts, Unit const& attacker, NativeUnit const& defender ) {
  int const attack_points  = attacker.desc().combat;
  int const defense_points = unit_attr( defender.type ).combat;
  return CombatEuroAttackBrave{
      .winner = choose_winner( ts.rand, attack_points,
                               defense_points ) };
}

CombatEuroAttackDwelling combat_euro_attack_dwelling(
    TS& ts, Unit const& attacker, Dwelling const& ) {
  int const attack_points = attacker.desc().combat;
  int const defense_points =
      unit_attr( e_native_unit_type::brave ).combat;
  return CombatEuroAttackDwelling{
      .winner = choose_winner( ts.rand, attack_points,
                               defense_points ) };
}

// FIXME: we can get rid of this since we now have a modifier
// that does this (the `ship_on_land` penalty).
CombatEuroAttackEuro make_combat_for_attacking_ship_on_land() {
  return CombatEuroAttackEuro{
      .winner   = e_combat_winner::attacker,
      .attacker = { .modifiers = {},
                    .weight    = 1.0,
                    .outcome =
                        EuroUnitCombatOutcome::destroyed{} },
      .defender = {
          .modifiers = {},
          .weight    = 0.0,
          .outcome   = EuroUnitCombatOutcome::no_change{} } };
}

} // namespace rn
