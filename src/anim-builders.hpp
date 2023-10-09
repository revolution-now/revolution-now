/****************************************************************
**anim-builders.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-28.
*
* Description: Builders for various animation sequences.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "anim-builder.rds.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/unit-id.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.rds.hpp"

namespace rn {

struct CombatBraveAttackColony;
struct CombatBraveAttackEuro;
struct CombatEuroAttackBrave;
struct CombatEuroAttackEuro;
struct CombatEuroAttackUndefendedColony;
struct CombatShipAttackShip;
struct DwellingCombatOutcome;
struct EuroUnitCombatOutcome;
struct SSConst;
struct Visibility;

enum class e_tribe;
enum class e_sfx;

// General euro-on-euro attack sequence.
AnimationSequence anim_seq_for_euro_attack_euro(
    SSConst const& ss, CombatEuroAttackEuro const& combat );

// General euro-on-brave attack sequence.
AnimationSequence anim_seq_for_euro_attack_brave(
    SSConst const& ss, CombatEuroAttackBrave const& combat );

// General brave-on-euro attack sequence.
AnimationSequence anim_seq_for_brave_attack_euro(
    SSConst const& ss, CombatBraveAttackEuro const& combat );

AnimationSequence anim_seq_for_brave_attack_colony(
    SSConst const& ss, CombatBraveAttackColony const& combat );

AnimationSequence anim_seq_for_naval_battle(
    SSConst const& ss, CombatShipAttackShip const& combat );

// This is a special case of an attack sequence where we are at-
// tacking an undefended colony (we may win and capture the
// colony, or lose and not capture it). Note that this will not
// include the final march into the colony.
AnimationSequence anim_seq_for_undefended_colony(
    SSConst const&                          ss,
    CombatEuroAttackUndefendedColony const& combat );

// For when a dwelling gets burned.
AnimationSequence anim_seq_for_dwelling_burn(
    SSConst const& ss, UnitId attacker_id,
    EuroUnitCombatOutcome const& attacker_outcome,
    NativeUnitId defender_id, DwellingId dwelling_id,
    DwellingCombatOutcome const& dwelling_destruction );

// Just slides a unit to an adjacent square without regard to
// what is on that square or if the square even exists (as it
// might not if a ship is moving off the map to sail the high
// seas).
AnimationSequence anim_seq_for_unit_move(
    SSConst const& ss, GenericUnitId unit_id,
    e_direction direction );

// In the case that a unit is boarding a ship we need a special
// animation which does the slide but also makes sure that the
// ship being boarded gets rendered on top of its stack so that
// the player knows which ship is being boarded.
AnimationSequence anim_seq_for_boarding_ship(
    SSConst const& ss, UnitId unit_id, UnitId ship_id,
    e_direction direction );

// General depixelation animation for unit.
AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, UnitId unit_id );

AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, NativeUnitId unit_id );

// Depixelation animation for euro units with a target.
AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, UnitId unit_id, e_unit_type target_type );

// Depixelation animation for native units with a target.
AnimationSequence anim_seq_for_unit_depixelation(
    SSConst const& ss, NativeUnitId unit_id,
    e_native_unit_type target_type );

// General enpixelation animation for unit.
AnimationSequence anim_seq_for_unit_enpixelation(
    SSConst const& ss, GenericUnitId unit_id );

// This one comes with a nice sound effect.
AnimationSequence anim_seq_for_treasure_enpixelation(
    SSConst const& ss, UnitId unit_id );

// Colony just disappears.
AnimationSequence anim_seq_for_colony_depixelation(
    SSConst const& ss, ColonyId colony_id );

// Native convert appears on the dwelling tile then slides to the
// attacker.
AnimationSequence anim_seq_for_convert_produced(
    SSConst const& ss, UnitId unit_id, e_direction direction );

// Generally units that are animated will be rendered on top of
// other (non-animated) units. But there are cases when a unit is
// not being animated but you still want it to appear at the top
// of its stack, and that's what this is for.
AnimationSequence anim_seq_unit_to_front(
    GenericUnitId unit_id );

// Depixelates an entire tribe; only used in cheat mode when
// deleting a tribe. The tribe must exist.
AnimationSequence anim_seq_for_cheat_tribe_destruction(
    SSConst const& ss, Visibility const& viz, e_tribe tribe );

// Just plays a sound.
AnimationSequence anim_seq_for_sfx( e_sfx sound );

} // namespace rn
