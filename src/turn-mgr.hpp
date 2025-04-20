/****************************************************************
**turn-mgr.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-20.
*
* Description: Helpers for turn processing.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "maybe.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct SSConst;

enum class e_nation;

/****************************************************************
** Public API.
*****************************************************************/
// Finds the first nation to move each turn.
maybe<e_nation> find_first_nation_to_move( SSConst const& ss );

// Given the current nation that just moved, finds the next one.
maybe<e_nation> find_next_nation_to_move(
    SSConst const& ss, e_nation const curr_nation );

} // namespace rn
