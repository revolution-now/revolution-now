/****************************************************************
**game-state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-02.
*
* Description: Holds what is saved when a game is saved.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"

// C++ standard library
#include <string>

namespace rn {

// This will run validation routines recursively over the entire
// save-game state. It is probably expensive to run.
valid_or<std::string> validate_game_state();

// This literally default constructs all save-game data struc-
// tures. The result will not be an officially valid game state,
// but it may be ok for some unit tests.
void default_construct_game_state();

/****************************************************************
** Fwd Decls
*****************************************************************/
// Each of the subsections of the top-level state struct get for-
// ward declared, that way a module that only needs access to one
// can just include it and isn't forced to pull in all of the
// other headers.
struct FormatVersion;
struct SettingsState;
struct EventsState;
struct UnitsState;
struct PlayersState;
struct TurnState;
struct ColoniesState;
struct LandViewState;
struct TerrainState;

struct TopLevelState;

/****************************************************************
** GameState
*****************************************************************/
struct GameState {
  static FormatVersion& version();
  static SettingsState& settings();
  static EventsState&   events();
  static UnitsState&    units();
  static PlayersState&  players();
  static TurnState&     turn();
  static ColoniesState& colonies();
  static LandViewState& land_view();
  static TerrainState&  terrain();

  static TopLevelState& top();
};

} // namespace rn
