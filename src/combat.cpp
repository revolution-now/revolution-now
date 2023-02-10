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
#include "alarm.hpp"
#include "irand.hpp"
#include "logger.hpp"
#include "missionary.hpp"
#include "promotion.hpp"
#include "society.rds.hpp"
#include "treasure.hpp"
#include "unit-mgr.hpp"

// config
#include "config/combat.rds.hpp"
#include "config/natives.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"
#include "ss/players.hpp"
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
  // Note: in the OG, experiments seem to indicate that the
  // braves will take muskets/horses only when they are the at-
  // tackers in the battle. Moreover, in that situation, they
  // will take the horses/muskets 100% of the time, assuming they
  // are available and the brave unit can accept them.
  //
  // TODO: when braves start to attack, they should take muskets
  // and horses whenever possible.
  //
  if( won )
    return NativeUnitCombatOutcome::no_change{};
  else
    // TODO: not really clear on the dynamics of the horses and
    // muskets held internally by a native tribe. There are some
    // preliminary investigation notes in
    // doc/native-muskets-horses.txt.
    return NativeUnitCombatOutcome::destroyed{
        .tribe_retains_horses  = false,
        .tribe_retains_muskets = false };
}

DwellingCombatOutcome::destruction compute_dwelling_destruction(
    SSConst const& ss, IRand& rand, Player const& player,
    Dwelling const& dwelling, bool missions_burned ) {
  DwellingCombatOutcome::destruction res;

  unordered_set<NativeUnitId> const& braves =
      ss.units.braves_for_dwelling( dwelling.id );
  // Note that this will not include the temporary brave created
  // as a target in the attack.
  res.braves_to_kill =
      vector<NativeUnitId>( braves.begin(), braves.end() );
  // So that unit tests are deterministic...
  sort( res.braves_to_kill.begin(), res.braves_to_kill.end() );

  // If we're not burning missions then we'll check to see if
  // there is a missionary to release. However, we will only re-
  // lease it if it is our missionary. If it is a foreign mis-
  // sionary then it will be left in the dwelling and it will be
  // destroyed along with the dwelling.
  if( !missions_burned ) {
    maybe<UnitId> const missionary_id =
        ss.units.missionary_from_dwelling( dwelling.id );
    if( missionary_id.has_value() ) {
      Unit const& missionary =
          ss.units.unit_for( *missionary_id );
      if( missionary.nation() == player.nation )
        res.missionary_to_release =
            ss.units.missionary_from_dwelling( dwelling.id );
    }
  }

  e_tribe const tribe = ss.natives.tribe_for( dwelling.id ).type;
  UNWRAP_CHECK( dwellings,
                ss.natives.dwellings_for_tribe( tribe ) );
  if( dwellings.size() == 1 ) {
    CHECK_EQ( *dwellings.begin(), dwelling.id );
    // Last dwelling of tribe; tribe is being wiped out.
    res.tribe_destroyed = tribe;
  }

  // Is there a treasure and, if so, how much.
  res.treasure_amount =
      treasure_from_dwelling( ss, rand, player, dwelling );

  return res;
}

DwellingCombatOutcome_t dwelling_combat_outcome(
    SSConst const& ss, IRand& rand, Player const& player,
    Dwelling const& dwelling, bool won, bool missions_burned ) {
  if( won ) return DwellingCombatOutcome::no_change{};

  // The dwelling has lost.
  bool const convert_produced = [&] {
    if( missions_burned ) return false;
    maybe<double> const convert_probability =
        probability_dwelling_produces_convert_on_attack(
            ss, player, dwelling.id );
    return convert_probability.has_value() &&
           rand.bernoulli( *convert_probability );
  }();

  // Check for simple population decrease.
  if( dwelling.population > 1 )
    return DwellingCombatOutcome::population_decrease{
        .convert_produced = convert_produced };

  // The dwelling has been destroyed.
  DwellingCombatOutcome::destruction destruction =
      compute_dwelling_destruction( ss, rand, player, dwelling,
                                    missions_burned );
  destruction.convert_produced = convert_produced;
  return destruction;
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
  Tribe const& tribe = ss_.natives.tribe_for( dwelling.id );
  TribeRelationship const& relationship =
      tribe.relationship[attacker.nation()];
  Player const& player =
      player_for_nation_or_die( ss_.players, attacker.nation() );
  double const attack_points = attacker.desc().combat;
  // When attacking a dwelling we always run the numbers as if we
  // are attacking a brave. Stength among tribes is varied both
  // by bonuses on dwelling type and also on population size.
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

  // Tribal alarm. If this is a capital then tribal alarm will be
  // increased more.
  int new_tribal_alarm = [&] {
    auto new_relationship = relationship;
    increase_tribal_alarm_from_attacking_dwelling(
        player, dwelling, new_relationship );
    return new_relationship.tribal_alarm;
  }();

  // The tribe can decide to burn the mission whether or not the
  // player wins.
  bool const missions_burned = [&] {
    if( player_missionaries_in_tribe( ss_, player, tribe.type )
            .empty() )
      return false;
    return should_burn_mission_on_attack( rand_,
                                          new_tribal_alarm );
  }();

  DwellingCombatOutcome_t const defender_outcome =
      dwelling_combat_outcome(
          ss_, rand_, player, dwelling,
          winner == e_combat_winner::defender, missions_burned );

  // If we're burning the capital then reduce alarm to content.
  if( dwelling.is_capital &&
      defender_outcome
          .holds<DwellingCombatOutcome::destruction>() )
    new_tribal_alarm = max_tribal_alarm_after_burning_capital();

  return CombatEuroAttackDwelling{
      .winner           = winner,
      .new_tribal_alarm = new_tribal_alarm,
      .missions_burned  = missions_burned,
      .attacker         = { .id        = attacker.id(),
                            .modifiers = {},
                            .weight    = attack_points,
                            .outcome   = attacker_outcome },
      .defender         = { .id        = dwelling.id,
                            .modifiers = {},
                            .weight    = defense_points,
                            .outcome   = defender_outcome } };
}

} // namespace rn
