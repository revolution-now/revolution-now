/****************************************************************
**charter.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-14.
*
* Description: Handles the "end of charter" mechanic.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IGui;
struct SSConst;

enum class e_player;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] int charter_end_year();

[[nodiscard]] bool should_warn_about_charter_end(
    SSConst const& ss, e_player player_type );

[[nodiscard]] bool should_end_charter( SSConst const& ss,
                                       e_player player_type );

wait<> end_charter_ui_seq( SSConst const& ss, IGui& gui,
                           e_player player_type );

} // namespace rn
