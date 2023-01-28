/****************************************************************
**combat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-14.
*
* Description: Handles statistics for individual combats.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "combat.rds.hpp"

namespace rn {

struct Dwelling;
struct NativeUnit;
struct TS;
struct Unit;

CombatEuroAttackEuro combat_euro_attack_euro(
    TS& ts, Unit const& attacker, Unit const& defender );

CombatEuroAttackBrave combat_euro_attack_brave(
    TS& ts, Unit const& attacker, NativeUnit const& defender );

CombatEuroAttackDwelling combat_euro_attack_dwelling(
    TS& ts, Unit const& attacker, Dwelling const& dwelling );

// In the original game a ship can be left on land after a colony
// is abandoned, but if a land unit then tries to attack it the
// game panics. We will handle it properly, but we don't want a
// normal battle to ensue, because then the player could "cheat"
// by leaving a bunch of fortified frigates on land that would be
// too strong for normal land units to take down. So in this
// game, when a ship is left on land, the player is given a mes-
// sage that they should move it off land as soon as possible be-
// cause it is vulnerable to attack. And by vulnerable, we mean
// that if it is attacked by a land unit (no matter how weak)
// then the land unit will always win. This will prevent the sce-
// nario above where the player accumultes ships on land as a
// "wall." This function thus returns a combat stats object where
// the attacker always wins.
//
// FIXME: we can get rid of this since we now have a modifier
// that does this (the `ship_on_land` penalty).
CombatEuroAttackEuro make_combat_for_attacking_ship_on_land();

} // namespace rn
