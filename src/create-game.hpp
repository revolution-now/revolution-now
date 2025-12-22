/****************************************************************
**create-game.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Handles the setup of a new game.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "game-setup.rds.hpp"
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
