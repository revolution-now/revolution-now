/****************************************************************
**game-options.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-12.
*
* Description: Implements the game options dialog box(es).
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct SS;
struct TS;

enum class e_game_flag_option;

wait<> open_game_options_box( SS& ss, TS& ts );

// Normal game code should call this when programmatically dis-
// abling a game option as it will ensure that the right side ef-
// fects get performed. The function returns the old value of the
// option, which might have been already off (false).
bool disable_game_option( SS& ss, TS& ts,
                          e_game_flag_option option );

} // namespace rn
