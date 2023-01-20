/****************************************************************
**enter-dwelling.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-06.
*
* Description: Drives the sequence of events that happen when a
*              unit attempts to enter a native dwelling.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "enter-dwelling.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Dwelling;
struct Planes;
struct Player;
struct SS;
struct SSConst;
struct TS;
struct TribeRelationship;
struct Unit;

/****************************************************************
** General.
*****************************************************************/
EnterNativeDwellingOptions enter_native_dwelling_options(
    SSConst const& ss, Player const& player,
    e_unit_type unit_type, Dwelling const& dwelling );

wait<e_enter_dwelling_option> present_dwelling_entry_options(
    SSConst const& ss, TS& ts,
    EnterNativeDwellingOptions const& options );

/****************************************************************
** Live Among the Natives.
*****************************************************************/
// Passing in the tribe relationship object helps to guarantee
// that the tribe has made contact; if it hasn't then we
// shouldn't even be calling this.
LiveAmongTheNatives_t compute_live_among_the_natives(
    SSConst const& ss, Dwelling const& dwelling,
    Unit const& unit );

wait<> do_live_among_the_natives(
    Planes& planes, TS& ts, Dwelling& dwelling,
    Player const& player, Unit& unit,
    LiveAmongTheNatives_t const& outcome );

/****************************************************************
** Speak with Chief
*****************************************************************/
SpeakWithChiefResult compute_speak_with_chief(
    SSConst const& ss, TS& ts, Dwelling const& dwelling,
    Unit const& unit );

wait<> do_speak_with_chief(
    Planes& planes, SS& ss, TS& ts, Dwelling& dwelling,
    Player& player, Unit& unit,
    SpeakWithChiefResult const& outcome );

/****************************************************************
** Attack Village
*****************************************************************/
AttackVillageResult compute_attack_village(
    SSConst const& ss, TS& ts, Player const& player,
    Dwelling const& dwelling, Unit const& attacker );

wait<> do_attack_village( Planes& planes, SS& ss, TS& ts,
                          Dwelling& dwelling, Player& player,
                          Unit&                      attacker,
                          AttackVillageResult const& outcome );

} // namespace rn
