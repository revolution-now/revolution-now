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
      builder.depixelate_unit( unit_id );
      break;
    case e::captured:
      builder.depixelate_unit( unit_id );
      break;
    case e::captured_and_demoted:
      builder.depixelate_unit( unit_id );
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
    EuroColonyWorkerCombatOutcome const& outcome ) {
  switch( outcome.to_enum() ) {
    using e = EuroColonyWorkerCombatOutcome::e;
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
    NativeUnitCombatOutcome const& outcome ) {
  switch( outcome.to_enum() ) {
    using e = NativeUnitCombatOutcome::e;
    case e::no_change:
      builder.front_unit( unit_id );
      break;
    case e::destroyed:
      builder.depixelate_unit( unit_id );
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

  // Phase 1: defender unit appears on top of stack and the at-
  // tacker slides toward it.
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
    // TODO: when the colony is depixelating, the road under it
    // is not removed during the animation (though it will later
    // be removed)... not yet sure if this is something that
    // should be changed. In the OG, it appears that it renders
    // the complete screen without the colony (and with the new
    // road configuration) and then depixelates the entire
    // screen, so the road configuration pixelates into the new
    // road configuration on the surrounding tiles as the colony
    // goes away. For the sake of polish, we should probably
    // replicate this, but it is probably not a high priority. It
    // will probably be non-trivial because we would need to
    // first remove the roads from the terrain by rerendering the
    // squares, then adding animations for roads, and pixelating
    // one road tile to another in the surrounding squares. NOTE:
    // this should also be done in the other colony depixelation
    // function, i.e., the one that gets called when the colony
    // starves or is abandoned. This would also allow us to not
    // have to clear the road first before the animation, which
    // would allow us to get rid of the function that does the
    // animated colony destruction.
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

  // Phase 1: defender unit appears above colony and the attacker
  // slides toward it.
  builder.front_unit( defender_id );
  builder.slide_unit( attacker_id, direction );
  builder.play_sound( e_sfx::move );

  // Phase 2: attacker unit, phantom defender unit, dwelling, and
  // any owned braves depixelate. If the attacker unit is pro-
  // moted then it will pixelate.
  builder.new_phase();
  // FIXME: there is a race in situations like the one below
  // where there are multiple things pixelating in the same
  // phase: they may end at slightly different times because they
  // start at slightly different times and because of how the
  // complicated nature of their timing mechanism interacts with
  // (discreet) frames. Need to figure out a general solution to
  // this. Adding a one frame delay to the end of each pixelation
  // animation may not help, since one could still finish before
  // the others. This issue can be demonstrated by adding a
  // `co_await 1_frames` onto the end of the dwelling depixela-
  // tion animation and then observing that, at the very end of a
  // dwelling burn, the phantom brave flashes on for one frame.
  add_attack_outcome_for_euro_unit( ss, builder, attacker_id,
                                    attacker_outcome );
  add_attack_outcome_for_native_unit(
      builder, defender_id,
      NativeUnitCombatOutcome::destroyed{} );
  // TODO: once we fix viewport scrolling in animations, we
  // should make sure to pan to the dwelling coord last so that
  // the viewport doesn't end up scrolling away from it just to
  // see one of the dwelling's brave depixelate, which can happen
  // if the brave has strayed far from the dwelling (if we can
  // only see one then it is better to see the dwelling).
  builder.depixelate_dwelling( dwelling_id );
  for( NativeUnitId const brave_id :
       dwelling_destruction.braves_to_kill ) {
    CHECK( brave_id != defender_id );
    builder.depixelate_unit( brave_id );
  }
  // Poor-man's volume increase.
  builder.play_sound( e_sfx::city_destroyed );
  builder.play_sound( e_sfx::city_destroyed );
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
  builder.pixelate_euro_unit_to_target( unit_id, target_type );
  // TODO: sound effect.
  return builder.result();
}

AnimationSequence anim_seq_for_unit_depixelation(
    NativeUnitId unit_id, e_native_unit_type target_type ) {
  AnimationBuilder builder;
  builder.pixelate_native_unit_to_target( unit_id, target_type );
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

AnimationSequence anim_seq_for_treasure_enpixelation(
    UnitId unit_id ) {
  AnimationBuilder builder;
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
    UnitId unit_id, e_direction direction ) {
  AnimationBuilder builder;

  // Phase 1: enpixelate convert.
  builder.enpixelate_unit( unit_id );

  // Phase 2: slide to attacker.
  builder.new_phase();
  builder.slide_unit( unit_id, direction );
  builder.play_sound( e_sfx::move );

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

AnimationSequence anim_seq_for_cheat_tribe_destruction(
    SSConst const& ss, Visibility const& viz, e_tribe tribe ) {
  AnimationBuilder builder;
  builder.play_sound( e_sfx::city_destroyed );

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
    builder.depixelate_unit( unit_id );

  return builder.result();
}

} // namespace rn
