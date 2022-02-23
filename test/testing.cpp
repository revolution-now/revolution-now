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
#include "game-state.hpp"
#include "lua.hpp"

using namespace std;

namespace testing {

fs::path const& data_dir() {
  static fs::path data{ "test/data" };
  return data;
}

void default_construct_all_game_state() {
  rn::default_construct_game_state();
  rn::lua_reload();
}

} // namespace testing
