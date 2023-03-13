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

wait<> open_game_options_box( SS& ss, TS& ts );

} // namespace rn
