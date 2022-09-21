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
#include "wait.hpp"

// base
#include "base/fs.hpp"
#include "base/vocab.hpp"

namespace rn {

struct RootState;
struct SSConst;
struct SS;
struct TS;

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

expect<fs::path> save_game( SSConst const& ss, TS& ts,
                            int slot );
expect<fs::path> load_game( SS& ss, TS& ts, int slot );

valid_or<std::string> save_game_to_rcl_file(
    RootState const& root, fs::path const& p,
    SaveGameOptions const& opts );
valid_or<std::string> load_game_from_rcl_file(
    RootState& root, fs::path const& p,
    SaveGameOptions const& opts );

void autosave( SSConst const& ss, TS& ts );

// Given the current turn index, this will tell us if it is time
// to autosave.
bool should_autosave( int turns );

// Opens the save-game box. Returns if the game was actually
// saved.
wait<bool> save_game_menu( SSConst const& ss, TS& ts );

// Opens the load-game box. Returns if a game was actually
// loaded.
wait<base::NoDiscard<bool>> load_game_menu( SS& ss, TS& ts );

// This is called when the user tries to do something that might
// leave the current game. It will check if the current game has
// unsaved changes and, if so, will ask the user if they want to
// save it. If the user does, then it will open the save-game di-
// alog box.
wait<> check_ask_save( SSConst const& ss, TS& ts );

} // namespace rn
