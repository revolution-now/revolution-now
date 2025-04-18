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

namespace ui {
enum class e_confirm;
}

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

void declare_independence( SS& ss, TS& ts, Player& player );

wait<> declare_independence_ui_sequence_post(
    SSConst const& ss, TS& ts, Player const& player );

} // namespace rn
