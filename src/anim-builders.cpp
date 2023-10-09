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
#include "visibility.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "visibility.rds.hpp"

using namespace std;

namespace rn {

namespace {

void add_attack_outcome_for_euro_unit(
    SSConst const& ss, AnimationBuilder& builder, UnitId unit_id,
    EuroUnitCombatOutcome const& outcome ) {
  switch( outcome.to_enum() ) {
    using e = EuroUnitCombatOutcome::e;
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
      builder.depixelate_euro_unit( unit_id );
      break;
    case e::captured:
      builder.depixelate_euro_unit( unit_id );
      break;
    case e::captured_and_demoted:
      builder.depixelate_euro_unit( unit_id );
      break;
    case e::promoted: {
      auto& o = outcome.get<EuroUnitCombatOutcome::promoted>();
      Unit const& unit = ss.units.unit_for( unit_id );
      CHECK_NEQ( unit.type_obj(), o.to );
      if( unit.type() == o.to.type() )
        // If the derived type is the same then we don't need to
        // do any depixelating. This can happen e.g. if a crimi-
        // nal/soldier gets promoted to a servant/soldier.
        builder.front_unit( unit_id );
      else
        builder.pixelate_euro_unit_to_target( unit_id,
                                              o.to.type() );
      break;
    }
    case e::demoted: {
      auto& o = outcome.get<EuroUnitCombatOutcome::demoted>();
      Unit const& unit = ss.units.unit_for( unit_id );
      CHECK_NEQ( unit.type(), o.to.type() );
      builder.pixelate_euro_unit_to_target( unit_id,
                                            o.to.type() );
      break;
    }
  }
}

void add_naval_attack_outcome_for_unit(
    AnimationBuilder& builder, UnitId unit_id,
    EuroNavalUnitCombatOutcome const& outcome ) {
  switch( outcome.to_enum() ) {
    using e = EuroNavalUnitCombatOutcome::e;
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
      builder.depixelate_euro_unit( unit_id );
      break;
    case e::sunk:
      builder.depixelate_euro_unit( unit_id );
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
    EuroColonyWorkerCombatOutcome const& outcome ) {
  switch( outcome.to_enum() ) {
    using e = EuroColonyWorkerCombatOutcome::e;
    case e::no_change:
      builder.front_unit( unit_id );
      break;
    case e::defeated:
      builder.depixelate_euro_unit( unit_id );
      break;
  }
}

void add_attack_outcome_for_native_unit(
    AnimationBuilder& builder, NativeUnitId unit_id,
    NativeUnitCombatOutcome const& outcome ) {
  switch( outcome.to_enum() ) {
    using e = NativeUnitCombatOutcome::e;
    case e::no_change:
      builder.front_unit( unit_id );
      break;
    case e::destroyed:
      builder.depixelate_native_unit( unit_id );
      break;
    case e::promoted: {
      auto& o = outcome.get<NativeUnitCombatOutcome::promoted>();
      builder.pixelate_native_unit_to_target( unit_id, o.to );
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
    using e = EuroUnitCombatOutcome::e;
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
    AnimationBuilder&            builder,
    CombatBraveAttackEuro const& combat ) {
  builder.play_sound(
      ( combat.winner == e_combat_winner::defender )
          ? e_sfx::attacker_lost
          : e_sfx::attacker_won );
}

void play_combat_outcome_sound(
    AnimationBuilder&              builder,
    CombatBraveAttackColony const& combat ) {
  if( combat.colony_destroyed ) {
    builder.play_sound( e_sfx::city_destroyed );
    return;
  }
  builder.play_sound(
      ( combat.winner == e_combat_winner::defender )
          ? e_sfx::attacker_lost
          : e_sfx::attacker_won );
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

// This is intended to be used on small collections of tiles
// (usually two) that are near each other, and attempts to leave
// the viewport panned in a way where both are visible. In theory
// that is not always possible when there are more than one, so
// we just pan to them in sequence. Visually this should be ok
// because, again, we are expecting that they will be near each
// other, and so panning to the first one will very likely also
// reveal the second one; but even if it doesn't, panning to the
// second should still leave the first one visible.
void ensure_tiles_visible( AnimationBuilder&    builder,
                           vector<Coord> const& tiles ) {
  for( Coord const tile : tiles )
    builder.ensure_tile_visible( tile );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
AnimationSequence anim_seq_for_euro_attack_euro(
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

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it. Most of the time the defender will
  // already be on top of the stack because the defender is
  // chosen as the unit in the stack with highest defense, which
  // is also the unit that will be rendered on top of a stack
  // normally. However, there are some cases where those two
  // might not correspond, since the default stack ordering goes
  // by raw combat value whereas the defender unit is chosen with
  // combat modifiers applied as well.
  builder.new_phase();
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

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it.
  builder.new_phase();
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

AnimationSequence anim_seq_for_euro_attack_brave(
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

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it.
  builder.new_phase();
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

AnimationSequence anim_seq_for_brave_attack_euro(
    SSConst const& ss, CombatBraveAttackEuro const& combat ) {
  NativeUnitId const attacker_id = combat.attacker.id;
  UnitId const       defender_id = combat.defender.id;
  Coord const        attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it. Most of the time the defender will
  // already be on top of the stack because the defender is
  // chosen as the unit in the stack with highest defense, which
  // is also the unit that will be rendered on top of a stack
  // normally. However, there are some cases where those two
  // might not correspond, since the default stack ordering goes
  // by raw combat value whereas the defender unit is chosen with
  // combat modifiers applied as well.
  builder.new_phase();
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: pixelations for both attacker and defender where
  // needed.
  builder.new_phase();
  add_attack_outcome_for_native_unit(
      builder, combat.attacker.id, combat.attacker.outcome );
  add_attack_outcome_for_euro_unit(
      ss, builder, combat.defender.id, combat.defender.outcome );
  play_combat_outcome_sound( builder, combat );

  return builder.result();
}

AnimationSequence anim_seq_for_brave_attack_colony(
    SSConst const& ss, CombatBraveAttackColony const& combat ) {
  NativeUnitId const attacker_id = combat.attacker.id;
  UnitId const       defender_id = combat.defender.id;
  Coord const        attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  Coord const colony_location = defender_coord;
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it.
  builder.new_phase();
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: pixelations for both attacker and defender where
  // needed, as well as depixelating the colony.
  builder.new_phase();
  add_attack_outcome_for_native_unit(
      builder, combat.attacker.id, combat.attacker.outcome );
  add_attack_outcome_for_euro_unit(
      ss, builder, combat.defender.id, combat.defender.outcome );
  if( combat.colony_destroyed ) {
    // If there are any units on the square then those need to be
    // hidden during the depixelation since they will be removed
    // from that square after the animation (ships will be sent
    // for repair and all other units, which will be
    // non-military, will be destroyed).
    vector<GenericUnitId> const units_to_hide = [&] {
      auto const& units_at_gate =
          ss.units.from_coord( colony_location );
      vector sorted_units( units_at_gate.begin(),
                           units_at_gate.end() );
      // The defender's animation is handled above.
      erase( sorted_units, defender_id );
      // For for determinism in unit tests.
      sort( sorted_units.begin(), sorted_units.end() );
      return sorted_units;
    }();
    for( GenericUnitId const id : units_to_hide )
      builder.hide_unit( id );
    builder.depixelate_colony( combat.colony_id );
  }
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
          .id              = combat.defender.id,
          .modifiers       = combat.defender.modifiers,
          .base_weight     = combat.defender.base_weight,
          .modified_weight = combat.defender.modified_weight,
          .outcome = EuroUnitCombatOutcome::no_change{} } };
  return anim_seq_for_euro_attack_euro( ss, new_combat );
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

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears above colony and the attacker
  // slides toward it.
  builder.new_phase();
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

AnimationSequence anim_seq_for_dwelling_burn(
    SSConst const& ss, UnitId attacker_id,
    EuroUnitCombatOutcome const& attacker_outcome,
    NativeUnitId defender_id, DwellingId dwelling_id,
    DwellingCombatOutcome const& dwelling_outcome ) {
  UNWRAP_CHECK(
      dwelling_destruction,
      dwelling_outcome
          .get_if<DwellingCombatOutcome::destruction>() );
  CHECK( attacker_outcome
             .holds<EuroUnitCombatOutcome::no_change>() ||
         attacker_outcome
             .holds<EuroUnitCombatOutcome::promoted>() );
  Coord const attacker_coord =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( direction,
                attacker_coord.direction_to( defender_coord ) );
  AnimationBuilder builder;

  // Phase 0: pan to battle site;
  ensure_tiles_visible( builder,
                        { attacker_coord, defender_coord } );

  // Phase 1: defender unit appears above colony and the attacker
  // slides toward it.
  builder.new_phase();
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: attacker unit, phantom defender unit, dwelling, and
  // any owned braves depixelate. If the attacker unit is pro-
  // moted then it will pixelate.
  builder.new_phase();
  add_attack_outcome_for_euro_unit( ss, builder, attacker_id,
                                    attacker_outcome );
  add_attack_outcome_for_native_unit(
      builder, defender_id,
      NativeUnitCombatOutcome::destroyed{} );
  builder.depixelate_dwelling( dwelling_id );
  for( NativeUnitId const brave_id :
       dwelling_destruction.braves_to_kill ) {
    CHECK( brave_id != defender_id );
    builder.depixelate_native_unit( brave_id );
  }
  // Poor-man's volume increase.
  builder.play_sound( e_sfx::city_destroyed );
  builder.play_sound( e_sfx::city_destroyed );
  builder.play_sound( e_sfx::city_destroyed );

  return builder.result();
}

AnimationSequence anim_seq_for_unit_move(
    SSConst const& ss, GenericUnitId unit_id,
    e_direction direction ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  Coord const      target = tile.moved( direction );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  ensure_tiles_visible( builder, { tile, target } );
  // Phase 1: slide.
  builder.new_phase();
  builder.slide_unit( unit_id, direction );
  builder.play_sound( e_sfx::move );
  return builder.result();
}

AnimationSequence anim_seq_for_boarding_ship(
    SSConst const& ss, UnitId unit_id, UnitId ship_id,
    e_direction direction ) {
  Coord const unit_tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  Coord const ship_tile =
      coord_for_unit_multi_ownership_or_die( ss, ship_id );
  CHECK_EQ( unit_tile.moved( direction ), ship_tile );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  ensure_tiles_visible( builder, { unit_tile, ship_tile } );
  // Phase 1: slide.
  builder.new_phase();
  builder.front_unit( ship_id );
  builder.slide_unit( unit_id, direction );
  builder.play_sound( e_sfx::move );
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, UnitId unit_id ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: depixelate.
  builder.new_phase();
  builder.depixelate_euro_unit( unit_id );
  builder.play_sound( e_sfx::attacker_lost );
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, NativeUnitId unit_id ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: depixelate.
  builder.new_phase();
  builder.depixelate_native_unit( unit_id );
  builder.play_sound( e_sfx::attacker_lost );
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, UnitId unit_id,
    e_unit_type target_type ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: pixelate.
  builder.new_phase();
  builder.pixelate_euro_unit_to_target( unit_id, target_type );
  // TODO: sound effect.
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, NativeUnitId unit_id,
    e_native_unit_type target_type ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: pixelate.
  builder.new_phase();
  builder.pixelate_native_unit_to_target( unit_id, target_type );
  // TODO: sound effect.
  return builder.result();
}

// General enpixelation animation for unit.
AnimationSequence anim_seq_for_unit_enpixelation(
    SSConst const& ss, GenericUnitId unit_id ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: enpixelate.
  builder.new_phase();
  builder.enpixelate_unit( unit_id );
  return builder.result();
}

AnimationSequence anim_seq_for_treasure_enpixelation(
    SSConst const& ss, UnitId unit_id ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: enpixelate.
  builder.new_phase();
  // TODO: this is not a high priority, but one thing that the
  // original game does that might be nice for us to do as an ef-
  // fect, if it is feasible, is that if a treasure enpixelates
  // over the location of a burned dwelling then any hidden ter-
  // rain around it that is revealed as a result will be revealed
  // in a pixelated way, along with the treasure's enpixelation
  // animation. The OG probably handles pixelation animations by
  // rerendering the entire screen in the new state and then pix-
  // elating the entire screen. This may be tricky for us to im-
  // plement given how we render terrain and obfuscation; need to
  // determine if it is worth the effort.
  builder.enpixelate_unit( unit_id );
  builder.play_sound( e_sfx::treasure );
  return builder.result();
}

AnimationSequence anim_seq_for_convert_produced(
    SSConst const& ss, UnitId unit_id, e_direction direction ) {
  Coord const tile =
      coord_for_unit_multi_ownership_or_die( ss, unit_id );
  Coord const target = tile.moved( direction );

  AnimationBuilder builder;
  // Phase 0: pan to site.
  ensure_tiles_visible( builder, { tile, target } );

  // Phase 1: enpixelate convert.
  builder.new_phase();
  builder.enpixelate_unit( unit_id );

  // Phase 2: slide to attacker.
  builder.new_phase();
  builder.slide_unit( unit_id, direction );
  builder.play_sound( e_sfx::move );

  return builder.result();
}

AnimationSequence anim_seq_for_colony_depixelation(
    SSConst const& ss, ColonyId colony_id ) {
  Coord const tile =
      ss.colonies.colony_for( colony_id ).location;
  AnimationBuilder builder;
  // Phase 0: pan to site.
  builder.ensure_tile_visible( tile );
  // Phase 1: depixelate colony.
  builder.new_phase();
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

// Note that we don't play any sounds here because this may be
// called multiple times in a loop to destroy multiple tribes.
AnimationSequence anim_seq_for_cheat_tribe_destruction(
    SSConst const& ss, Visibility const& viz, e_tribe tribe ) {
  AnimationBuilder builder;

  // Dwellings.
  gfx::rect_iterator ri( ss.terrain.world_rect_tiles() );
  for( Coord const tile : ri ) {
    switch( viz.visible( tile ) ) {
      case e_tile_visibility::visible_and_clear: {
        maybe<DwellingId> const dwelling_id =
            ss.natives.maybe_dwelling_from_coord( tile );
        if( !dwelling_id.has_value() ) break;
        if( ss.natives.tribe_for( *dwelling_id ).type != tribe )
          continue;
        builder.depixelate_dwelling( *dwelling_id );
        break;
      }
      case e_tile_visibility::visible_with_fog: {
        maybe<FogSquare const&> fog_square =
            viz.fog_square_at( tile );
        if( !fog_square.has_value() ) continue;
        maybe<FogDwelling> const& fog_dwelling =
            fog_square->dwelling;
        if( !fog_dwelling.has_value() ) continue;
        if( fog_dwelling->tribe != tribe ) continue;
        builder.depixelate_fog_dwelling( tile );
        break;
      }
      case e_tile_visibility::hidden:
        break;
    }
  }

  // Braves.
  auto const&          native_units = ss.units.native_all();
  vector<NativeUnitId> native_unit_ids;
  native_unit_ids.reserve( native_units.size() );
  for( auto [unit_id, p_state] : native_units ) {
    UNWRAP_CHECK( world,
                  p_state->ownership
                      .get_if<NativeUnitOwnership::world>() );
    e_tribe const tribe_type =
        ss.natives.tribe_for( world.dwelling_id ).type;
    if( tribe_type != tribe ) continue;
    native_unit_ids.push_back( unit_id );
  }
  sort( native_unit_ids.begin(), native_unit_ids.end() );
  for( NativeUnitId const unit_id : native_unit_ids )
    // This should do the right thing whether the unit is visible
    // or not (if it's not, it won't be animated, and there are
    // no fogged units).
    builder.depixelate_native_unit( unit_id );

  return builder.result();
}

AnimationSequence anim_seq_for_sfx( e_sfx sound ) {
  AnimationBuilder builder;
  builder.play_sound( sound );
  return builder.result();
}

} // namespace rn
