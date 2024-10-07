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
#include "damaged.hpp"
#include "irand.hpp"
#include "logger.hpp"
#include "missionary.hpp"
#include "promotion.hpp"
#include "society.rds.hpp"
#include "treasure.hpp"
#include "unit-mgr.hpp"
#include "unit-transformation.hpp"

// config
#include "config/combat.rds.hpp"
#include "config/natives.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

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

EuroUnitCombatOutcome euro_unit_combat_outcome(
    SSConst const& ss, IRand& rand, Unit const& unit,
    Society const& society_of_opponent, Coord opponent_coord,
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
    using e = UnitDeathAction::e;
    case e::capture: {
      if( society_of_opponent.holds<Society::native>() )
        // When the natives defeat a unit that is otherwise cap-
        // turable then it just gets destroyed. E.g. a colonist,
        // treasure, wagon train, etc.
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

NativeUnitCombatOutcome native_unit_lost_combat_outcome(
    IRand& rand, NativeUnit const& unit ) {
  // The brave has been destroyed, so compute if the tribe will
  // retain its horses or muskets (if it has any). It has been
  // confirmed in the OG that the brave can retain both if it has
  // both (i.e. mounted warrior) and the behavior seems to be the
  // same whether the brave was the attacker or defender. Not
  // sure if the game applies the horses/muskets probabilities
  // separately in the case of a mounted warrior, but that
  // doesn't seemt to matter since they are both 50%, so on av-
  // erage we'd get the same outcome either way.
  auto retains = [&]( e_brave_equipment eq ) {
    return config_natives.arms.equipment[unit.type][eq] &&
           rand.bernoulli(
               config_natives.arms.retention_after_death[eq]
                   .probability );
  };
  return NativeUnitCombatOutcome::destroyed{
    .tribe_retains_horses = retains( e_brave_equipment::horses ),
    .tribe_retains_muskets =
        retains( e_brave_equipment::muskets ) };
}

// In the OG braves will take muskets/horses only when they are
// the attackers in the battle and, moreover, in that situation,
// they will take them 100% of the time, assuming they are avail-
// able and the brave unit can accept them.
NativeUnitCombatOutcome native_unit_won_attacking_combat_outcome(
    NativeUnit const& native_unit, Unit const& defender,
    EuroUnitCombatOutcome const& euro_outcome ) {
  UnitTransformation const trans = [&] {
    SWITCH( euro_outcome ) {
      CASE( captured ) { SHOULD_NOT_BE_HERE; }
      CASE( captured_and_demoted ) { SHOULD_NOT_BE_HERE; }
      CASE( no_change ) { SHOULD_NOT_BE_HERE; }
      CASE( promoted ) { SHOULD_NOT_BE_HERE; }
      CASE( demoted ) {
        UNWRAP_CHECK( trans,
                      query_unit_transformation(
                          defender.composition(), demoted.to ) );
        return trans;
      }
      CASE( destroyed ) {
        return strip_to_base_type( defender.composition() );
      }
    }
  }();
  auto const& equipment = config_natives.arms.equipment;
  refl::enum_map<e_brave_equipment, bool> const new_eq = [&] {
    auto eq = equipment[native_unit.type];
    if( trans.modifier_deltas[e_unit_type_modifier::horses] ==
        e_unit_type_modifier_delta::del )
      eq[e_brave_equipment::horses] = true;
    if( trans.modifier_deltas[e_unit_type_modifier::muskets] ==
        e_unit_type_modifier_delta::del )
      eq[e_brave_equipment::muskets] = true;
    return eq;
  }();
  e_native_unit_type const new_type =
      find_brave( new_eq[e_brave_equipment::muskets],
                  new_eq[e_brave_equipment::horses] );
  if( new_type != native_unit.type ) {
    bool const gained_horses =
        !equipment[native_unit.type]
                  [e_brave_equipment::horses] &&
        equipment[new_type][e_brave_equipment::horses];
    return NativeUnitCombatOutcome::promoted{
      .to = new_type, .tribe_gains_horse_herd = gained_horses };
  }
  return NativeUnitCombatOutcome::no_change{};
}

NativeUnitCombatOutcome
native_unit_won_defending_combat_outcome() {
  return NativeUnitCombatOutcome::no_change{};
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
  unordered_set<DwellingId> const& dwellings =
      ss.natives.dwellings_for_tribe( tribe );
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

DwellingCombatOutcome dwelling_combat_outcome(
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
  EuroUnitCombatOutcome const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::european{ .nation = defender.nation() },
          defender_coord, winner == e_combat_winner::attacker );
  EuroUnitCombatOutcome const defender_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, defender,
          Society::european{ .nation = attacker.nation() },
          attacker_coord, winner == e_combat_winner::defender );
  return CombatEuroAttackEuro{
    .winner   = winner,
    .attacker = { .id              = attacker.id(),
                  .modifiers       = {},
                  .base_weight     = attack_points,
                  .modified_weight = attack_points,
                  .outcome         = attacker_outcome },
    .defender = { .id              = defender.id(),
                  .modifiers       = {},
                  .base_weight     = defense_points,
                  .modified_weight = defense_points,
                  .outcome         = defender_outcome } };
}

void RealCombat::set_sunk_or_damaged(
    Unit const& unit, Coord unit_coord,
    EuroNavalUnitCombatOutcome& outcome, bool sinks ) {
  if( sinks ) {
    outcome = EuroNavalUnitCombatOutcome::sunk{};
    return;
  }
  // Damaged. Try to find a port to repair the ship.
  maybe<ShipRepairPort> const port = find_repair_port_for_ship(
      ss_, unit.nation(), unit_coord );
  if( port.has_value() )
    outcome =
        EuroNavalUnitCombatOutcome::damaged{ .port = *port };
  else
    // This will happen after independence is declared and the
    // player has no colonies with a Drydock, in which case there
    // is no place to send the ship for repair, so it just sinks.
    outcome = EuroNavalUnitCombatOutcome::sunk{};
}

/*
Naval Combat Mechanics from the OG:

  1. Let attack=can_attack?combat:0; if attack for defender is >=
     attack for attacker then there is no evade. Otherwise, the
     evade probability will be computed according to the formula
     m_def/(m_att+m_def), where m_def is the number of
     start-of-turn movement points of the attacker plus one and
     m_def is the equivalent for the defender. In other words, we
     take the max movement points for the units and add one (note
     that we don't use the current number of movement points).
     Note that the Ferdinand Magellan movement point bonus is
     also applied. Also, there is a bonus that is applied for the
     privateer (whether attacker or defender) whereby it gets is
     effective movement points doubled for the purposes of the
     calculation. That makes it much more difficult to evade a
     privateer and makes the privateer better at evading when at-
     tacked. This doesn't appear to be documented anywhere; it,
     just as with the above formula in general, was determined
     through empirical observation over many thousands of trials.
  2. If either there is no evade possible or if the evade dice
     roll yielded no evade, then we proceed to the combat analy-
     sis.
  3. Weigh each of the combat factors, giving the 50% bonus to
     the attacker and 50% Francis Drake bonus (where applicable),
     as usual. Whichever ship wins will be unchanged (or moved,
     in the case of a winning attacker), and the losing ship will
     be either damaged or sunk.
  4. The probability of sinking is equal to
     "guns"_winner/("guns"_winner+"hull"_loser). I.e., the proba-
     bility that the "guns of the winner can penetrate the hull
     of the loser."
  5. Note: the OG makes a significant adjustment to the sinking
     probability (if not a complete override) having to do with
     the size of each player's naval fleet, possibly in relation
     to the strength of the other player's naval fleet, in that a
     ship is much more likely to sink if the player's navy is
     large and/or they already have many damaged ships. This is
     likely done to discourage or put a check on large navies.
     But, we will not replicate that in this game. See the
     naval-mechanics.txt doc file for more info on this, though
     the exact mechanism used by the OG seems convoluted and is
     not fully understood.
*/
CombatShipAttackShip RealCombat::ship_attack_ship(
    Unit const& attacker, Unit const& defender ) {
  CHECK( attacker.desc().ship );
  CHECK( defender.desc().ship );
  Player const& attacking_player =
      player_for_nation_or_die( ss_.players, attacker.nation() );
  Player const& defending_player =
      player_for_nation_or_die( ss_.players, attacker.nation() );
  Coord const attacker_coord =
      ss_.units.coord_for( attacker.id() );
  Coord const defender_coord =
      ss_.units.coord_for( defender.id() );
  UNWRAP_CHECK( attacker_ship_combat,
                attacker.desc().ship_combat_extra );
  UNWRAP_CHECK( defender_ship_combat,
                defender.desc().ship_combat_extra );
  int const attacker_attack_strength =
      attacker.desc().can_attack ? attacker.desc().combat : 0;
  int const defender_attack_strength =
      defender.desc().can_attack ? defender.desc().combat : 0;
  CHECK_GT( attacker_attack_strength, 0 );
  // The attacker obviously doesn't evade; this is just the name
  // used for the attacker's weight in the evade calculation.
  int const attacker_evade_bonus =
      attacker_ship_combat.evasion_bonus ? 2 : 1;
  int const defender_evade_bonus =
      defender_ship_combat.evasion_bonus ? 2 : 1;
  MvPoints const attacker_evade =
      ( movement_points( attacking_player, attacker.type() ) +
        MovementPoints( 1 ) ) *
      attacker_evade_bonus;
  MvPoints const defender_evade =
      ( movement_points( defending_player, defender.type() ) +
        MovementPoints( 1 ) ) *
      defender_evade_bonus;
  double const attacker_combat = attacker.desc().combat;
  double const defender_combat = defender.desc().combat;
  // Try evade.
  bool const evaded = [&] {
    bool const try_evade =
        defender_attack_strength < attacker_attack_strength;
    if( !try_evade ) return false;
    return rand_.bernoulli(
        double( defender_evade.atoms() ) /
        ( defender_evade.atoms() + attacker_evade.atoms() ) );
  }();

  CHECK( attacker_evade.atoms() % 3 == 0 );
  CHECK( defender_evade.atoms() % 3 == 0 );
  // Fill out what we can so far.
  CombatShipAttackShip res{
    .winner       = nothing,
    .sink_weights = nothing,
    .attacker     = { .id           = attacker.id(),
                      .modifiers    = {},
                      .evade_weight = attacker_evade.atoms() / 3,
                      .base_combat_weight     = attacker_combat,
                      .modified_combat_weight = attacker_combat,
                      .outcome                = {} },
    .defender     = { .id           = defender.id(),
                      .modifiers    = {},
                      .evade_weight = defender_evade.atoms() / 3,
                      .base_combat_weight     = defender_combat,
                      .modified_combat_weight = defender_combat,
                      .outcome                = {} } };

  if( evaded ) {
    res.winner = nothing;
    res.attacker.outcome =
        EuroNavalUnitCombatOutcome::no_change{};
    res.defender.outcome =
        EuroNavalUnitCombatOutcome::no_change{};
    return res;
  }

  // Not evaded, so someone wins.
  bool const attacker_wins =
      rand_.bernoulli( double( attacker_combat ) /
                       ( attacker_combat + defender_combat ) );
  res.winner = attacker_wins ? e_combat_winner::attacker
                             : e_combat_winner::defender;
  Unit const& winner_unit = attacker_wins ? attacker : defender;
  Unit const& loser_unit  = attacker_wins ? defender : attacker;
  Coord const loser_coord =
      attacker_wins ? defender_coord : attacker_coord;
  NavalCombatStats& loser_stats =
      attacker_wins ? res.defender : res.attacker;
  NavalCombatStats& winner_stats =
      attacker_wins ? res.attacker : res.defender;
  CHECK( &winner_stats != &loser_stats );
  CHECK( &winner_unit != &loser_unit );

  // Set the outcome of the winner.
  if( winner_unit.id() == attacker.id() )
    winner_stats.outcome = EuroNavalUnitCombatOutcome::moved{
      .to = defender_coord };
  else
    winner_stats.outcome =
        EuroNavalUnitCombatOutcome::no_change{};

  // Now we can compute the sink weights since we know whose guns
  // and whose hull strength we need.
  auto does_sink = [this]( Sinking sinking ) {
    return rand_.bernoulli( double( sinking.guns ) /
                            ( sinking.guns + sinking.hull ) );
  };

  int const guns = attacker_wins ? attacker_ship_combat.guns
                                 : defender_ship_combat.guns;
  int const hull = attacker_wins ? defender_ship_combat.hull
                                 : attacker_ship_combat.hull;

  res.sink_weights.emplace() = { .guns = guns, .hull = hull };
  bool const loser_sinks     = does_sink( *res.sink_weights );

  // Set the outcome of the loser.
  set_sunk_or_damaged( loser_unit, loser_coord,
                       loser_stats.outcome, loser_sinks );

  // In the OG, when a defending ship loses in combat, not only
  // will it get either sunk or damaged, but all other ships on
  // its square will get sunk or damaged as well, and the damaged
  // vs. sink outcome is computed independently for each ship.
  //
  // Note that this has to be the case if the attacking (and win-
  // ning) ship slides onto the defender's square after winning.
  if( attacker_wins ) {
    vector<UnitId> ships_on_defender_square;
    for( GenericUnitId const generic_unit_id :
         ss_.units.from_coord( defender_coord ) )
      ships_on_defender_square.push_back(
          ss_.units.check_euro_unit( generic_unit_id ) );
    // For determinism in unit tests.
    sort( ships_on_defender_square.begin(),
          ships_on_defender_square.end() );
    // At least the defender will be on the square.
    CHECK_GT( ships_on_defender_square.size(), 0u );
    for( UnitId const unit_id : ships_on_defender_square ) {
      if( unit_id == defender.id() ) continue;
      Unit const& other_ship = ss_.units.unit_for( unit_id );
      UNWRAP_CHECK( other_ship_combat,
                    other_ship.desc().ship_combat_extra );
      int const  other_hull = other_ship_combat.hull;
      auto const sinking =
          Sinking{ .guns = guns, .hull = other_hull };
      bool const                 sinks = does_sink( sinking );
      EuroNavalUnitCombatOutcome outcome;
      set_sunk_or_damaged( other_ship, defender_coord, outcome,
                           sinks );
      res.affected_defender_units[unit_id] =
          AffectedNavalDefender{ .id           = unit_id,
                                 .sink_weights = sinking,
                                 .outcome      = outcome };
    }
  }

  return res;
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
  EuroUnitCombatOutcome const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::european{ .nation = defender.nation() },
          defender_coord, winner == e_combat_winner::attacker );
  auto const defender_outcome =
      [&]() -> EuroColonyWorkerCombatOutcome {
    if( winner == e_combat_winner::attacker )
      return EuroColonyWorkerCombatOutcome::defeated{};
    else
      return EuroColonyWorkerCombatOutcome::no_change{};
  }();
  return CombatEuroAttackUndefendedColony{
    .winner    = winner,
    .colony_id = colony.id,
    .attacker  = { .id              = attacker.id(),
                   .modifiers       = {},
                   .base_weight     = attack_points,
                   .modified_weight = attack_points,
                   .outcome         = attacker_outcome },
    .defender  = { .id              = defender.id(),
                   .modifiers       = {},
                   .base_weight     = defense_points,
                   .modified_weight = defense_points,
                   .outcome         = defender_outcome } };
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
  EuroUnitCombatOutcome const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::native{
            .tribe = tribe_type_for_unit( ss_, defender ) },
          defender_coord, winner == e_combat_winner::attacker );
  NativeUnitCombatOutcome const defender_outcome =
      ( winner == e_combat_winner::defender )
          ? native_unit_won_defending_combat_outcome()
          : native_unit_lost_combat_outcome( rand_, defender );
  return CombatEuroAttackBrave{
    .winner   = winner,
    .attacker = { .id              = attacker.id(),
                  .modifiers       = {},
                  .base_weight     = attack_points,
                  .modified_weight = attack_points,
                  .outcome         = attacker_outcome },
    .defender = { .id              = defender.id,
                  .modifiers       = {},
                  .base_weight     = defense_points,
                  .modified_weight = defense_points,
                  .outcome         = defender_outcome } };
}

CombatBraveAttackEuro RealCombat::brave_attack_euro(
    NativeUnit const& attacker, Unit const& defender ) {
  double const attack_points = unit_attr( attacker.type ).combat;
  double const defense_points = defender.desc().combat;
  Coord const  attacker_coord =
      ss_.units.coord_for( attacker.id );
  e_combat_winner const winner =
      choose_winner( rand_, attack_points, defense_points );
  EuroUnitCombatOutcome const defender_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, defender,
          Society::native{
            .tribe = tribe_type_for_unit( ss_, attacker ) },
          attacker_coord, winner == e_combat_winner::defender );
  NativeUnitCombatOutcome const attacker_outcome =
      ( winner == e_combat_winner::attacker )
          ? native_unit_won_attacking_combat_outcome(
                attacker, defender, defender_outcome )
          : native_unit_lost_combat_outcome( rand_, attacker );
  return CombatBraveAttackEuro{
    .winner   = winner,
    .attacker = { .id              = attacker.id,
                  .modifiers       = {},
                  .base_weight     = attack_points,
                  .modified_weight = attack_points,
                  .outcome         = attacker_outcome },
    .defender = { .id              = defender.id(),
                  .modifiers       = {},
                  .base_weight     = defense_points,
                  .modified_weight = defense_points,
                  .outcome         = defender_outcome } };
}

// The mechanics for a brave attacking a colony in the OG (which
// we replicate here) are different than for most other types of
// combat:
//
//   1. After attacking a colony the brave always gets destroyed
//      (population of its dwelling is unchanged).
//   2. If it is the last colony then the brave always attacks
//      with zero strength, so the defender never loses, though
//      there may be side effects.
//   3. If it is not the last colony then the attacker attacks
//      with non-zero strength and can win the battle in the
//      sense that the defender may lose and/or the colony may be
//      destroyed. That said, the brave still gets destroyed. The
//      brave is considered to have "won" though, and the combat
//      effects messages reflect that.
//
// The OG implements (2) above by effectively setting the Brave's
// attack to zero when there is only one colony remaining.
CombatBraveAttackColony RealCombat::brave_attack_colony(
    NativeUnit const& attacker, Unit const& defender,
    Colony const& colony ) {
  if( ss_.colonies.for_nation( colony.nation ).size() > 1 ) {
    // When the player has more than one colony then the battle
    // between the units proceeds normally for the most part.
    // That is, the brave can win and demote a soldier, destroy a
    // colonist, destroy the colony, cause the defender to get
    // promoted, etc. However, regardless of the outcome, the
    // brave always gets destroyed (this distinguishes it from a
    // normal battle). That said, if the brave wins the combat,
    // it is still considered to be the winner even though it
    // will be destroyed as a result.
    CombatBraveAttackEuro const units_combat =
        brave_attack_euro( attacker, defender );
    // Note that this colony destruction happens even when the
    // colony posesses a fortress.
    bool const colony_destroyed =
        ( units_combat.winner == e_combat_winner::attacker &&
          !is_military_unit( defender.type() ) );
    auto defender_stats = units_combat.defender;
    if( colony_destroyed )
      defender_stats.outcome =
          EuroUnitCombatOutcome::destroyed{};
    auto attacker_stats_modified = units_combat.attacker;
    // Even though the brave can win here, it will still be de-
    // stroyed, so we compute the outcome as if it lost.
    attacker_stats_modified.outcome =
        native_unit_lost_combat_outcome( rand_, attacker );
    return { .winner           = units_combat.winner,
             .colony_id        = colony.id,
             .colony_destroyed = colony_destroyed,
             .attacker         = attacker_stats_modified,
             .defender         = defender_stats };
  }
  // This is the player's first/last colony; in this case, the OG
  // handles the side effects as usual (such as e.g. stealing
  // money or damaging buildings), but always causes the brave to
  // lose the combat, so i.e. no colonies or military units are
  // ever lost or damaged, and the colony is never lost. Note
  // that this is a bit different from the case of multiple
  // colonies above, even though the brave is always destroyed in
  // both cases; the difference is that in the case of a single
  // colony the brave can destroy the colony or demote a soldier.
  double const attack_points  = 0; // OG has this zero.
  double const defense_points = defender.desc().combat;
  Coord const  attacker_coord =
      ss_.units.coord_for( attacker.id );
  NativeUnitCombatOutcome const attacker_outcome =
      native_unit_lost_combat_outcome( rand_, attacker );
  // It seems that in the OG, even though the defender can never
  // lose, it can still be promoted.
  EuroUnitCombatOutcome const defender_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, defender,
          Society::native{
            .tribe = tribe_type_for_unit( ss_, attacker ) },
          attacker_coord, /*won=*/true );
  return { .winner           = e_combat_winner::defender,
           .colony_id        = colony.id,
           .colony_destroyed = false,
           .attacker         = { .id              = attacker.id,
                                 .modifiers       = {},
                                 .base_weight     = attack_points,
                                 .modified_weight = attack_points,
                                 .outcome         = attacker_outcome },
           .defender         = { .id              = defender.id(),
                                 .modifiers       = {},
                                 .base_weight     = defense_points,
                                 .modified_weight = defense_points,
                                 .outcome         = defender_outcome } };
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
  EuroUnitCombatOutcome const attacker_outcome =
      euro_unit_combat_outcome(
          ss_, rand_, attacker,
          Society::native{
            .tribe = ss_.natives.tribe_for( dwelling.id ).type },
          defender_coord, winner == e_combat_winner::attacker );

  // The tribe can decide to burn the mission whether or not the
  // player wins.
  bool const missions_burned = [&] {
    if( player_missionaries_in_tribe( ss_, player, tribe.type )
            .empty() )
      return false;
    return should_burn_mission_on_attack(
        rand_, relationship.tribal_alarm );
  }();

  DwellingCombatOutcome const defender_outcome =
      dwelling_combat_outcome(
          ss_, rand_, player, dwelling,
          winner == e_combat_winner::defender, missions_burned );

  return CombatEuroAttackDwelling{
    .winner          = winner,
    .missions_burned = missions_burned,
    .attacker        = { .id              = attacker.id(),
                         .modifiers       = {},
                         .base_weight     = attack_points,
                         .modified_weight = attack_points,
                         .outcome         = attacker_outcome },
    .defender        = { .id              = dwelling.id,
                         .modifiers       = {},
                         .base_weight     = defense_points,
                         .modified_weight = defense_points,
                         .outcome         = defender_outcome } };
}

} // namespace rn
