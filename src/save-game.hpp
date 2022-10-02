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

// This will open the load-game dialog box and will allow the
// player to choose a slot, but will not actually do the loading.
// This is useful in cases where we want to immediately display
// the box to the user, but we want to do the loading elsewhere.
wait<maybe<int>> choose_load_slot( TS& ts );

// Given a slot (which must contain a save file) load it. Will
// return if the load actually succeeded.
wait<base::NoDiscard<bool>> load_slot( SS& ss, TS& ts,
                                       int slot );

// This is called when the user tries to do something that might
// leave the current game. It will check if the current game has
// unsaved changes and, if so, will ask the user if they want to
// save it. If the user does, then it will open the save-game di-
// alog box. If the user escapes out of that dialog box (i.e.,
// does not explicitly select "no") then this function will re-
// turn false, since that can be useful to the caller, since it
// may indicate that the user is trying to abort whatever process
// prompted the save-game box to appear in the first place.
wait<base::NoDiscard<bool>> check_ask_save( SSConst const& ss,
                                            TS&            ts );

/****************************************************************
** Low level methods.
*****************************************************************/
// These do the actual saving without any user interaction; they
// are public mostly for testing. These shouldn't be called di-
// rectly.

expect<fs::path> save_game( SSConst const& ss, TS& ts,
                            int slot );
expect<fs::path> load_game( SS& ss, TS& ts, int slot );

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

valid_or<std::string> save_game_to_rcl_file(
    RootState const& root, fs::path const& p,
    SaveGameOptions const& opts );
valid_or<std::string> load_game_from_rcl_file(
    RootState& root, fs::path const& p,
    SaveGameOptions const& opts );

} // namespace rn
