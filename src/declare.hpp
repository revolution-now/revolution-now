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
struct SS;
struct SSConst;
struct TS;
struct Player;

enum class e_nation;

/****************************************************************
** Public API.
*****************************************************************/
base::valid_or<e_declare_rejection> can_declare_independence(
    SSConst const& ss, Player const& player );

wait<> show_declare_rejection_msg( IGui& gui,
                                   e_declare_rejection reason );

wait<ui::e_confirm> ask_declare( IGui& gui,
                                 Player const& player );

maybe<e_nation> player_that_declared( SSConst const& ss );

wait<> declare_independence_ui_sequence_pre(
    SSConst const& ss, TS& ts, Player const& player );

[[nodiscard]] DeclarationResult declare_independence(
    SS& ss, TS& ts, Player& player );

wait<> declare_independence_ui_sequence_post(
    SSConst const& ss, TS& ts, Player const& player,
    DeclarationResult const& decl_res );

} // namespace rn
