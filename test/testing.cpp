/****************************************************************
**testing.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-05.
*
* Description: Common definitions for unit tests.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "lua.hpp"
#include "save-game.hpp"

using namespace std;

namespace rn::testing {

fs::path const& data_dir() {
  static fs::path data{ "test/data" };
  return data;
}

void reset_savegame_state() {
  CHECK_XP( rn::reset_savegame_state() );
  rn::lua::reload();
}

void default_construct_all_game_state() {
  rn::default_construct_savegame_state();
  rn::lua::reload();
}

} // namespace rn::testing
