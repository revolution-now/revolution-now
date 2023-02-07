/****************************************************************
**anim-builders.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-28.
*
* Description: Builders for various animation sequences.
*
*****************************************************************/
#include "anim-builders.hpp"

// Revolution Now
#include "anim-builder.hpp"
#include "icombat.rds.hpp"
#include "unit-mgr.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

void add_attack_outcome_for_euro_unit(
    SSConst const& ss, AnimationBuilder& builder, UnitId unit_id,
    EuroUnitCombatOutcome_t const& outcome ) {
  switch( outcome.to_enum() ) {
    using namespace EuroUnitCombatOutcome;
    case e::no_change:
      // Add this so that if the other unit in the battle needs
      // to animate then this unit will remain visible during
      // that time. If not (i.e., neither unit has an attack re-
      // sult animation and so both get "front"ed), there is no
      // harm in adding this since the animation phase in ques-
      // tion will consist only of fronts which, although
      // non-terminating, will not be co-awaited on, so the en-
      // tire phase will pass by instantaneously.
      builder.front_unit( unit_id );
      break;
    case e::destroyed:
      builder.depixelate_unit( unit_id );
      break;
    case e::captured:
      builder.depixelate_unit( unit_id );
      break;
    case e::captured_and_demoted:
      builder.depixelate_unit( unit_id );
      break;
    case e::promoted: {
      auto&       o    = outcome.get<promoted>();
      Unit const& unit = ss.units.unit_for( unit_id );
      CHECK_NEQ( unit.type_obj(), o.to );
      if( unit.type() == o.to.type() )
        // If the derived type is the same then we don't need to
        // do any depixelating. This can happen e.g. if a crimi-
        // nal/soldier gets promoted to a servant/soldier.
        builder.front_unit( unit_id );
      else
        builder.depixelate_euro_unit_to_target( unit_id,
                                                o.to.type() );
      break;
    }
    case e::demoted: {
      auto&       o    = outcome.get<demoted>();
      Unit const& unit = ss.units.unit_for( unit_id );
      CHECK_NEQ( unit.type(), o.to.type() );
      builder.depixelate_euro_unit_to_target( unit_id,
                                              o.to.type() );
      break;
    }
  }
}

void add_naval_attack_outcome_for_unit(
    AnimationBuilder& builder, UnitId unit_id,
    EuroNavalUnitCombatOutcome_t const& outcome ) {
  switch( outcome.to_enum() ) {
    using namespace EuroNavalUnitCombatOutcome;
    case e::no_change:
      // Add this so that if the other unit in the battle needs
      // to animate then this unit will remain visible during
      // that time. If not (i.e., neither unit has an attack re-
      // sult animation and so both get "front"ed), there is no
      // harm in adding this since the animation phase in ques-
      // tion will consist only of fronts, which are of zero du-
      // ration and so will pass by invisibly.
      builder.front_unit( unit_id );
      break;
    case e::damaged:
      builder.depixelate_unit( unit_id );
      break;
    case e::sunk:
      builder.depixelate_unit( unit_id );
      break;
    case e::moved:
      // None here, since this will be done in a subsequent
      // phase.
      break;
  }
}

// This is for a colony worker unit in an undefended colony.
void add_colony_worker_attack_outcome_for_unit(
    AnimationBuilder& builder, UnitId unit_id,
    EuroColonyWorkerCombatOutcome_t const& outcome ) {
  switch( outcome.to_enum() ) {
    using namespace EuroColonyWorkerCombatOutcome;
    case e::no_change:
      builder.front_unit( unit_id );
      break;
    case e::defeated:
      builder.depixelate_unit( unit_id );
      break;
  }
}

void add_attack_outcome_for_native_unit(
    AnimationBuilder& builder, NativeUnitId unit_id,
    NativeUnitCombatOutcome_t const& outcome ) {
  switch( outcome.to_enum() ) {
    using namespace NativeUnitCombatOutcome;
    case e::no_change:
      builder.front_unit( unit_id );
      break;
    case e::destroyed:
      builder.depixelate_unit( unit_id );
      break;
    case e::promoted: {
      auto& o = outcome.get<promoted>();
      builder.depixelate_native_unit_to_target( unit_id, o.to );
      break;
    }
  }
}

void play_combat_outcome_sound(
    AnimationBuilder& builder, SSConst const& ss,
    CombatEuroAttackEuro const& combat ) {
  if( combat.winner == e_combat_winner::defender ) {
    builder.play_sound( e_sfx::attacker_lost );
    return;
  }
  // Defender lost.
  switch( combat.defender.outcome.to_enum() ) {
    using namespace EuroUnitCombatOutcome;
    case e::no_change:
      break;
    case e::destroyed: {
      bool const sunken_ship =
          ss.units.unit_for( combat.defender.id ).desc().ship;
      if( sunken_ship )
        builder.play_sound( e_sfx::sunk_ship );
      else
        builder.play_sound( e_sfx::attacker_won );
      break;
    }
    case e::captured:
      builder.play_sound( e_sfx::attacker_won );
      break;
    case e::captured_and_demoted:
      builder.play_sound( e_sfx::attacker_won );
      break;
    case e::promoted:
      // If the defender lost then it should not be promoted.
      SHOULD_NOT_BE_HERE;
    case e::demoted:
      builder.play_sound( e_sfx::attacker_won );
      break;
  }
}

void play_combat_outcome_sound(
    AnimationBuilder&           builder,
    CombatShipAttackShip const& combat ) {
  switch( combat.outcome ) {
    case e_naval_combat_outcome::evade:
      // TODO: better sound here?
      builder.play_sound( e_sfx::move );
      break;
    case e_naval_combat_outcome::damaged:
      builder.play_sound( e_sfx::attacker_won );
      break;
    case e_naval_combat_outcome::sunk:
      builder.play_sound( e_sfx::sunk_ship );
      break;
  }
}

void play_combat_outcome_sound(
    AnimationBuilder&            builder,
    CombatEuroAttackBrave const& combat ) {
  switch( combat.winner ) {
    case e_combat_winner::attacker:
      builder.play_sound( e_sfx::attacker_won );
      break;
    case e_combat_winner::defender:
      builder.play_sound( e_sfx::attacker_lost );
      break;
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
AnimationSequence anim_seq_for_attack_euro(
    SSConst const& ss, CombatEuroAttackEuro const& combat ) {
  UnitId const attacker_id = combat.attacker.id;
  UnitId const defender_id = combat.defender.id;
  Coord const  attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it. Most of the time the defender will
  // already be on top of the stack because the defender is
  // chosen as the unit in the stack with highest defense, which
  // is also the unit that will be rendered on top of a stack
  // normally. However, there are some cases where those two
  // might not correspond, since the default stack ordering goes
  // by raw combat value whereas the defender unit is chosen with
  // combat modifiers applied as well.
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: pixelations for both attacker and defender where
  // needed.
  builder.new_phase();
  add_attack_outcome_for_euro_unit(
      ss, builder, combat.attacker.id, combat.attacker.outcome );
  add_attack_outcome_for_euro_unit(
      ss, builder, combat.defender.id, combat.defender.outcome );
  play_combat_outcome_sound( builder, ss, combat );

  return builder.result();
}

AnimationSequence anim_seq_for_naval_battle(
    SSConst const& ss, CombatShipAttackShip const& combat ) {
  UnitId const attacker_id = combat.attacker.id;
  UnitId const defender_id = combat.defender.id;
  Coord const  attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it.
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: pixelations for both attacker and defender where
  // needed.
  builder.new_phase();
  add_naval_attack_outcome_for_unit( builder, combat.attacker.id,
                                     combat.attacker.outcome );
  add_naval_attack_outcome_for_unit( builder, combat.defender.id,
                                     combat.defender.outcome );
  play_combat_outcome_sound( builder, combat );

  // Phase 3: if the attacker wins (and the defender is sunk or
  // damaged) then the attacker will move into the defender's
  // square.
  if( combat.attacker.outcome
          .holds<EuroNavalUnitCombatOutcome::moved>() ) {
    CHECK_EQ( combat.attacker.outcome
                  .get<EuroNavalUnitCombatOutcome::moved>()
                  .to,
              defender_coord );
    builder.new_phase();
    builder.hide_unit( defender_id );
    builder.slide_unit( attacker_id, direction );
    builder.play_sound( e_sfx::move );
  }

  return builder.result();
}

AnimationSequence anim_seq_for_attack_brave(
    SSConst const& ss, CombatEuroAttackBrave const& combat ) {
  UnitId const       attacker_id = combat.attacker.id;
  NativeUnitId const defender_id = combat.defender.id;
  Coord const        attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it.
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: pixelations for both attacker and defender where
  // needed.
  builder.new_phase();
  add_attack_outcome_for_euro_unit(
      ss, builder, combat.attacker.id, combat.attacker.outcome );
  add_attack_outcome_for_native_unit(
      builder, combat.defender.id, combat.defender.outcome );
  play_combat_outcome_sound( builder, combat );

  return builder.result();
}

static AnimationSequence anim_seq_for_lost_colony_capture(
    SSConst const&                          ss,
    CombatEuroAttackUndefendedColony const& combat ) {
  CombatEuroAttackEuro const new_combat{
      .winner   = combat.winner,
      .attacker = combat.attacker,
      .defender = {
          .id        = combat.defender.id,
          .modifiers = combat.defender.modifiers,
          .weight    = combat.defender.weight,
          .outcome   = EuroUnitCombatOutcome::no_change{} } };
  return anim_seq_for_attack_euro( ss, new_combat );
}

AnimationSequence anim_seq_for_undefended_colony(
    SSConst const&                          ss,
    CombatEuroAttackUndefendedColony const& combat ) {
  UnitId const attacker_id = combat.attacker.id;
  UnitId const defender_id = combat.defender.id;
  if( combat.winner == e_combat_winner::defender )
    // Just a normal battle that the attacker has lost, so dele-
    // gate to the normal attack animation sequence.
    return anim_seq_for_lost_colony_capture( ss, combat );
  Coord const attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 1: defender unit appears above colony and the attacker
  // slides toward it.
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: attacker unit remains visible while the defender
  // unit depixelates. If the attacker unit is promoted then it
  // will pixelate.
  builder.new_phase();
  add_attack_outcome_for_euro_unit(
      ss, builder, combat.attacker.id, combat.attacker.outcome );
  add_colony_worker_attack_outcome_for_unit(
      builder, combat.defender.id, combat.defender.outcome );
  builder.play_sound( e_sfx::city_destroyed );

  return builder.result();
}

AnimationSequence anim_seq_for_unit_move(
    GenericUnitId unit_id, e_direction direction ) {
  AnimationBuilder builder;
  builder.slide_unit( unit_id, direction );
  builder.play_sound( e_sfx::move );
  return builder.result();
}

AnimationSequence anim_seq_for_boarding_ship(
    UnitId unit_id, UnitId ship_id, e_direction direction ) {
  AnimationBuilder builder;
  builder.front_unit( ship_id );
  builder.slide_unit( unit_id, direction );
  builder.play_sound( e_sfx::move );
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    GenericUnitId unit_id ) {
  AnimationBuilder builder;
  builder.depixelate_unit( unit_id );
  builder.play_sound( e_sfx::attacker_lost );
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    UnitId unit_id, e_unit_type target_type ) {
  AnimationBuilder builder;
  builder.depixelate_euro_unit_to_target( unit_id, target_type );
  // TODO: sound effect.
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    NativeUnitId unit_id, e_native_unit_type target_type ) {
  AnimationBuilder builder;
  builder.depixelate_native_unit_to_target( unit_id,
                                            target_type );
  // TODO: sound effect.
  return builder.result();
}

// General enpixelation animation for unit.
AnimationSequence anim_seq_for_unit_enpixelation(
    GenericUnitId unit_id ) {
  AnimationBuilder builder;
  builder.enpixelate_unit( unit_id );
  return builder.result();
}

AnimationSequence anim_seq_for_colony_depixelation(
    ColonyId colony_id ) {
  AnimationBuilder builder;
  builder.depixelate_colony( colony_id );
  builder.play_sound( e_sfx::city_destroyed );
  return builder.result();
}

AnimationSequence anim_seq_unit_to_front(
    GenericUnitId unit_id ) {
  AnimationBuilder builder;
  builder.front_unit( unit_id );
  return builder.result();
}

AnimationSequence anim_seq_unit_to_front_non_background(
    GenericUnitId unit_id ) {
  AnimationBuilder builder;
  builder.front_unit_non_background( unit_id );
  return builder.result();
}

} // namespace rn
