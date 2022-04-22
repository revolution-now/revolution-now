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
#include "map-updater.hpp"

// base
#include "base/fs.hpp"

namespace rn {

enum e_savegame_verbosity {
  // This will write every field within the data structure when
  // saving the game, even if those fields have their
  // default-initialized values. This will produce a larger save
  // file, but makes all fields visible and explicit.
  full,
  // This will suppress writing fields that have their default
  // values. It produces a smaller save file, but sometimes it
  // can be harder to read.
  compact
};

struct SaveGameOptions {
  e_savegame_verbosity verbosity = e_savegame_verbosity::full;
};

expect<fs::path> save_game( int slot );
expect<fs::path> load_game( IMapUpdater& map_updater, int slot );

valid_or<std::string> save_game_to_rcl_file(
    fs::path const& p, SaveGameOptions const& opts );
valid_or<std::string> load_game_from_rcl_file(
    IMapUpdater& map_updater, fs::path const& p,
    SaveGameOptions const& opts );

} // namespace rn
