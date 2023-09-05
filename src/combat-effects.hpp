/****************************************************************
**combat-effects.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-07-22.
*
* Description: Performs actions that need to happen when a unit
*              finishes a battle.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "combat-effects.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/unit-id.hpp"

namespace rn {

struct CombatBraveAttackColony;
struct CombatBraveAttackEuro;
struct CombatColonyArtilleryAttackShip;
struct CombatEuroAttackBrave;
struct CombatEuroAttackDwelling;
struct CombatEuroAttackEuro;
struct CombatEuroAttackUndefendedColony;
struct CombatShipAttackShip;
struct EuroNavalUnitCombatOutcome;
struct EuroUnitCombatOutcome;
struct IMind;
struct NativeUnit;
struct NativeUnitCombatOutcome;
struct SS;
struct SSConst;
struct TS;
struct Unit;

/****************************************************************
** Producing combat effects messages.
*****************************************************************/
CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatEuroAttackEuro const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatBraveAttackColony const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatBraveAttackEuro const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatEuroAttackBrave const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const&                         ss,
    CombatColonyArtilleryAttackShip const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatEuroAttackDwelling const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const&                          ss,
    CombatEuroAttackUndefendedColony const& combat );

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatShipAttackShip const& combat );

/****************************************************************
** Showing combat effects messages.
*****************************************************************/
MixedCombatEffectsMessages combine_combat_effects_msgs(
    CombatEffectsMessages const& msg );

wait<> show_combat_effects_msg(
    MixedCombatEffectsMessages const& msgs, IMind& attacker_mind,
    IMind& defender_mind );

/****************************************************************
** Performing combat effects.
*****************************************************************/
void perform_euro_unit_combat_effects(
    SS& ss, TS& ts, Unit& unit,
    EuroUnitCombatOutcome const& outcome );

void perform_native_unit_combat_effects(
    SS& ss, NativeUnit& unit,
    NativeUnitCombatOutcome const& outcome );

// Note that we take the opponent as an ID because the opponent
// may have been sunk by this point and so may not exist, and so
// it is safer to take a reference to it even though this func-
// tion will only make use of it when the opponent unit exists.
void perform_naval_unit_combat_effects(
    SS& ss, TS& ts, Unit& unit, UnitId opponent_id,
    EuroNavalUnitCombatOutcome const& outcome );

} // namespace rn
