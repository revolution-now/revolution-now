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
#include "aliases.hpp"
#include "errors.hpp"

namespace rn {

expect<fs::path> save_game( int slot );
expect<fs::path> load_game( int slot );

// Temporary.
expect<> reset_savegame_state();

/****************************************************************
** Testing
*****************************************************************/
void test_save_game();

} // namespace rn
