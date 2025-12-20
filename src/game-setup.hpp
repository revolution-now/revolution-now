/****************************************************************
**game-setup.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Handles the setup of a new game.
*
*****************************************************************/
#pragma once

// rds
#include "game-setup.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct SS;

/****************************************************************
** Public API.
*****************************************************************/
wait_bool create_game_from_setup( SS& ss, IRand& rand,
                                  lua::state& lua,
                                  GameSetup const& setup );

} // namespace rn
