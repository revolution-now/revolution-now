/****************************************************************
**game-state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-02.
*
* Description: Holds the serializable state of a game.
*
*****************************************************************/
#include "game-state.hpp"

// Revolution Now
#include "gs-root.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

RootState& g_state() {
  static RootState top;
  return top;
}

}

/****************************************************************
** GameState
*****************************************************************/
FormatVersion& GameState::version() {
  return g_state().version();
}

EventsState& GameState::events() { return g_state().events(); }

SettingsState& GameState::settings() {
  return g_state().settings();
}

UnitsState& GameState::units() { return g_state().units(); }

PlayersState& GameState::players() {
  return g_state().players();
}

TurnState& GameState::turn() { return g_state().turn(); }

ColoniesState& GameState::colonies() {
  return g_state().colonies();
}

LandViewState& GameState::land_view() {
  return g_state().land_view();
}

TerrainState& GameState::terrain() {
  return g_state().terrain();
}

RootState& GameState::root() { return g_state(); }

/****************************************************************
** Public API
*****************************************************************/
valid_or<std::string> validate_game_state() {
  // TODO: just do a quick-and-dirty recursive approach within
  // this file.
  NOT_IMPLEMENTED;
}

void default_construct_game_state() { g_state() = {}; }

} // namespace rn
