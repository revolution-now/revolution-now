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

struct EuroNavalUnitCombatOutcome;
struct EuroUnitCombatOutcome;
struct IEuroMind;
struct NativeUnit;
struct NativeUnitCombatOutcome;
struct SS;
struct SSConst;
struct TS;
struct Unit;

/****************************************************************
** Producing combat effects messages.
*****************************************************************/
CombatEffectsMessage euro_unit_combat_effects_msg(
    Unit const& unit, EuroUnitCombatOutcome const& outcome );

CombatEffectsMessage native_unit_combat_effects_msg(
    SSConst const& ss, NativeUnit const& unit,
    NativeUnitCombatOutcome const& outcome );

CombatEffectsMessage naval_unit_combat_effects_msg(
    SSConst const& ss, Unit const& unit, Unit const& opponent,
    EuroNavalUnitCombatOutcome const& outcome );

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

/****************************************************************
** Showing combat effects messages.
*****************************************************************/
struct EuroCombatEffectsMessage {
  IEuroMind&           mind;
  CombatEffectsMessage msg = {};
};

struct NativeCombatEffectsMessage {
  CombatEffectsMessage msg = {};
};

wait<> show_combat_effects_messages_euro_euro(
    EuroCombatEffectsMessage const& attacker,
    EuroCombatEffectsMessage const& defender );

wait<> show_combat_effects_messages_euro_attacker_only(
    EuroCombatEffectsMessage const& attacker );

wait<> show_combat_effects_messages_euro_native(
    EuroCombatEffectsMessage const&   euro,
    NativeCombatEffectsMessage const& native );

} // namespace rn
