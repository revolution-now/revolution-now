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

} // namespace rn
