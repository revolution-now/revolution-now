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
#include "promotion.hpp"
#include "society.rds.hpp"
#include "unit-mgr.hpp"

// config
#include "config/combat.rds.hpp"
#include "config/natives.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// config
#include "config/unit-type.hpp"

using namespace std;

namespace rn {

namespace {

e_combat_winner choose_winner( IRand& rand,
                               double attacker_weight,
                               double defender_weight ) {
  CHECK_GE( attacker_weight, 0 );
  CHECK_GE( defender_weight, 0 );
  if( attacker_weight == 0 && defender_weight == 0 )
    return e_combat_winner::attacker;
  double const winning_probability =
      attacker_weight / ( attacker_weight + defender_weight );
  CHECK_LE( winning_probability, 1.0 );
  lg.info( "winning probability: {}", winning_probability );
  return rand.bernoulli( winning_probability )
             ? e_combat_winner::attacker
             : e_combat_winner::defender;
}

maybe<UnitType> should_promote_euro_unit( SSConst const& ss,
                                          IRand&         rand,
                                          Unit const&    unit ) {
  if( maybe<e_unit_activity> const activity =
          current_activity_for_unit( ss.units, ss.colonies,
                                     unit.id() );
      activity != e_unit_activity::fighting )
    return nothing;
  expect<UnitComposition> const promoted =
      promoted_from_activity( unit.composition(),
                              e_unit_activity::fighting );
  if( !promoted.has_value() ) return nothing;
  UNWRAP_CHECK( player, ss.players.players[unit.nation()] );
  bool should_promote = true;
  if( promoted->type_obj().unit_type_modifiers().contains(
          e_unit_type_modifier::independence ) &&
      player.revolution_status < e_revolution_status::declared )
    // Don't allow promoting to a continental unit before inde-
    // pendence is declared.
    should_promote = false;
  if( !player.fathers.has[e_founding_father::george_washington] )
    should_promote =
        should_promote &&
        rand.bernoulli(
            config_combat.promotion_in_combat.probability );
  if( !should_promote ) return nothing;
  return promoted->type_obj();
}

EuroUnitCombatOutcome_t euro_unit_combat_outcome(
    SSConst const& ss, IRand& rand, Unit const& unit,
    Society_t const& society_of_opponent, Coord opponent_coord,
    bool won ) {
  if( won ) {
    if( maybe<UnitType> promoted_type =
            should_promote_euro_unit( ss, rand, unit );
        promoted_type.has_value() )
      return EuroUnitCombatOutcome::promoted{
          .to = *promoted_type };
    return EuroUnitCombatOutcome::no_change{};
  }

  switch( unit.desc().on_death.to_enum() ) {
    using namespace UnitDeathAction;
    case e::capture: {
      if( society_of_opponent.holds<Society::native>() )
        // When the natives defeat a unit that is otherwise cap-
        // turable then it just gets destroyed. E.g. a colonist,
        // treasure, wagon train, etc. FIXME: this is not yet
        // unit tested because we don't have a function braves
        // attacking european units.
        return EuroUnitCombatOutcome::destroyed{};
      UNWRAP_CHECK(
          euro_society,
          society_of_opponent.get_if<Society::european>() );
      return EuroUnitCombatOutcome::captured{
          .new_nation = euro_society.nation,
          .new_coord  = opponent_coord };
    }
    case e::capture_and_demote: {
      if( society_of_opponent.holds<Society::native>() )
        // See analogous comment above.
        return EuroUnitCombatOutcome::destroyed{};
      UNWRAP_CHECK(
          euro_society,
          society_of_opponent.get_if<Society::european>() );
      UNWRAP_CHECK( capture_demoted,
                    on_capture_demoted_type( unit.type_obj() ) );
      return EuroUnitCombatOutcome::captured_and_demoted{
          .to         = capture_demoted,
          .new_nation = euro_society.nation,
          .new_coord  = opponent_coord };
    }
    case e::destroy:
      return EuroUnitCombatOutcome::destroyed{};
    case e::demote: {
      UNWRAP_CHECK( to,
                    on_death_demoted_type( unit.type_obj() ) );
      return EuroUnitCombatOutcome::demoted{ .to = to };
    }
    case e::naval: {
      SHOULD_NOT_BE_HERE;
    }
  }
}

NativeUnitCombatOutcome_t native_unit_combat_outcome(
    NativeUnit const&, bool won ) {
  if( won )
    // TODO: allow for the brave to obtain horses/muskets.
    return NativeUnitCombatOutcome::no_change{};
  else
    // TODO: determine if the tribe retains the horses/muskets.
    return NativeUnitCombatOutcome::destroyed{
        .tribe_retains_horses  = false,
        .tribe_retains_muskets = false };
}

DwellingCombatOutcome_t dwelling_combat_outcome( bool won ) {
  if( won )
    return DwellingCombatOutcome::no_change{};
  else
    return DwellingCombatOutcome::defeated{};
}

} // namespace

CombatEuroAttackEuro RealCombat::euro_attack_euro(
    Unit const& attacker, Unit const& defender ) {
  CHECK( !attacker.desc().ship );
  double const attack_points  = attacker.desc().combat;
  double const defense_points = defender.desc().combat;
  Coord const  attacker_coord =
      ss_.units.coord_for( attacker.id() );
  Coord const defender_coord =
      ss_.units.coord_for( defender.id() );
  e_combat_winner const winner =
      choose_winner( rand_, attack_points, defense_points );
  EuroUnitCombatOutcome_t const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::european{ .nation = defender.nation() },
          defender_coord, winner == e_combat_winner::attacker );
  EuroUnitCombatOutcome_t const defender_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, defender,
          Society::european{ .nation = attacker.nation() },
          attacker_coord, winner == e_combat_winner::defender );
  return CombatEuroAttackEuro{
      .winner   = winner,
      .attacker = { .id        = attacker.id(),
                    .modifiers = {},
                    .weight    = attack_points,
                    .outcome   = attacker_outcome },
      .defender = { .id        = defender.id(),
                    .modifiers = {},
                    .weight    = defense_points,
                    .outcome   = defender_outcome } };
}

CombatShipAttackShip RealCombat::ship_attack_ship(
    Unit const&, Unit const& ) {
  // We need to work out the precise details of naval combat.
  // This has been started; see doc/col1-fighting.txt for data
  // collected thus far.
  NOT_IMPLEMENTED;
}

CombatEuroAttackUndefendedColony
RealCombat::euro_attack_undefended_colony(
    Unit const& attacker, Unit const& defender,
    Colony const& colony ) {
  // By assumption here the colony is undefended, so we should
  // not be fighting against a military unit.
  CHECK( !is_military_unit( defender.type() ) );
  double const          attack_points  = attacker.desc().combat;
  double const          defense_points = defender.desc().combat;
  Coord const           defender_coord = colony.location;
  e_combat_winner const winner =
      choose_winner( rand_, attack_points, defense_points );
  EuroUnitCombatOutcome_t const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::european{ .nation = defender.nation() },
          defender_coord, winner == e_combat_winner::attacker );
  auto const defender_outcome =
      [&]() -> EuroColonyWorkerCombatOutcome_t {
    if( winner == e_combat_winner::attacker )
      return EuroColonyWorkerCombatOutcome::defeated{};
    else
      return EuroColonyWorkerCombatOutcome::no_change{};
  }();
  return CombatEuroAttackUndefendedColony{
      .winner    = winner,
      .colony_id = colony.id,
      .attacker  = { .id        = attacker.id(),
                     .modifiers = {},
                     .weight    = attack_points,
                     .outcome   = attacker_outcome },
      .defender  = { .id        = defender.id(),
                     .modifiers = {},
                     .weight    = defense_points,
                     .outcome   = defender_outcome } };
}

CombatEuroAttackBrave RealCombat::euro_attack_brave(
    Unit const& attacker, NativeUnit const& defender ) {
  double const attack_points = attacker.desc().combat;
  double const defense_points =
      unit_attr( defender.type ).combat;
  Coord const defender_coord =
      ss_.units.coord_for( defender.id );
  e_combat_winner const winner =
      choose_winner( rand_, attack_points, defense_points );
  EuroUnitCombatOutcome_t const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::native{ .tribe =
                               tribe_for_unit( ss_, defender ) },
          defender_coord, winner == e_combat_winner::attacker );
  NativeUnitCombatOutcome_t const defender_outcome =
      native_unit_combat_outcome(
          defender, winner == e_combat_winner::defender );
  return CombatEuroAttackBrave{
      .winner   = winner,
      .attacker = { .id        = attacker.id(),
                    .modifiers = {},
                    .weight    = attack_points,
                    .outcome   = attacker_outcome },
      .defender = { .id        = defender.id,
                    .modifiers = {},
                    .weight    = defense_points,
                    .outcome   = defender_outcome } };
}

CombatEuroAttackDwelling RealCombat::euro_attack_dwelling(
    Unit const& attacker, Dwelling const& dwelling ) {
  double const attack_points = attacker.desc().combat;
  double const defense_points =
      unit_attr( e_native_unit_type::brave ).combat;
  Coord const defender_coord =
      ss_.natives.coord_for( dwelling.id );
  e_combat_winner const winner =
      choose_winner( rand_, attack_points, defense_points );
  EuroUnitCombatOutcome_t const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::native{
              .tribe =
                  ss_.natives.tribe_for( dwelling.id ).type },
          defender_coord, winner == e_combat_winner::attacker );
  DwellingCombatOutcome_t const defender_outcome =
      dwelling_combat_outcome( winner ==
                               e_combat_winner::defender );
  return CombatEuroAttackDwelling{
      .winner   = winner,
      .attacker = { .id        = attacker.id(),
                    .modifiers = {},
                    .weight    = attack_points,
                    .outcome   = attacker_outcome },
      .defender = { .id        = dwelling.id,
                    .modifiers = {},
                    .weight    = defense_points,
                    .outcome   = defender_outcome } };
}

} // namespace rn
