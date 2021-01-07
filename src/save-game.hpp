/****************************************************************
**save-game.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-18.
*
* Description: Interface for saving and loading a game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "expect.hpp"

// base
#include "base/fs.hpp"

namespace rn {

expect<fs::path, generic_err> save_game( int slot );
expect<fs::path, generic_err> load_game( int slot );

// This will load an empty game (i.e., will run the sync and
// validation methods for each state module).
valid_or<generic_err> reset_savegame_state();

// This literally default constructs all save-game data struc-
// tures. The result will not be an officially valid game state,
// but it may be ok for some unit tests.
void default_construct_savegame_state();

/****************************************************************
** Testing
*****************************************************************/
void test_save_game();

} // namespace rn
