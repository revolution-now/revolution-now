/****************************************************************
**declare.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-28.
*
* Description: Declaration of independence action.
*
*****************************************************************/
#pragma once

// rds
#include "declare.rds.hpp"

// Revolution Now
#include "wait.hpp"

// FIXME: need this because we can't forward declare the
// e_confirm enum due to its [[nodiscard]] attribute:
//   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88893
#include "ui-enums.rds.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct IGui;
struct IEngine;
struct SS;
struct SSConst;
struct TerrainConnectivity;
struct TS;
struct Player;

enum class e_player;

/****************************************************************
** Public API.
*****************************************************************/
maybe<e_player> human_player_that_declared( SSConst const& ss );

base::valid_or<e_declare_rejection> can_declare_independence(
    SSConst const& ss, Player const& player );

wait<> show_declare_rejection_msg( SSConst const& ss,
                                   Player const& player,
                                   IGui& gui,
                                   e_declare_rejection reason );

wait<ui::e_confirm> ask_declare(
    SSConst const& ss, IGui& gui,
    TerrainConnectivity const& connectivity,
    Player const& player );

wait<> declare_independence_ui_sequence_pre(
    SSConst const& ss, TS& ts, Player const& player );

[[nodiscard]] DeclarationResult declare_independence(
    IEngine& engine, SS& ss, TS& ts, Player& player );

wait<> declare_independence_ui_sequence_post(
    SSConst const& ss, TS& ts, Player const& player,
    DeclarationResult const& decl_res );

// The OG schedules a few different events to happen on the turns
// following the one where declaration happened. If T is the turn
// where the declaration happened:
//
//   T + 0: First REF troops arrive.
//   T + 1: Continental armies promoted.
//   T + 2: Hint messages messages given to player regarding
//          battle tactics and the intervention force.
//
// This function tries to determine which turn we're on relative
// to the one where we declared independence. This would be made
// easier if we were to just record the turn where independence
// was declared, but the OG does not record that, so would lose
// interconvertibility of sav files. The OG appears to track this
// by having separate flags for the events that happen on each of
// the above turns. So we just replicate those same flags and use
// them to piece together where we are.
e_turn_after_declaration post_declaration_turn(
    Player const& player );

} // namespace rn
