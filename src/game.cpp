/****************************************************************
**game.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-11.
*
* Description: Overall game flow of an individual game.
*
*****************************************************************/
#include "game.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "conductor.hpp"
#include "game-state.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "map-updater.hpp"
#include "plane.hpp"
#include "renderer.hpp" // FIXME: remove
#include "save-game.hpp"
#include "turn.hpp"

// luapp
#include "luapp/state.hpp"

using namespace std;

namespace rn {

namespace {

enum class e_game_module_tune_points {
  start_game //
};

void play( e_game_module_tune_points tune ) {
  switch( tune ) {
    case e_game_module_tune_points::start_game:
      conductor::play_request(
          conductor::e_request::fife_drum_happy,
          conductor::e_request_probability::always );
      break;
  }
}

wait<> turn_loop( PlayersState&        players_state,
                  TerrainState const&  terrain_state,
                  UnitsState&          units_state,
                  SettingsState const& settings,
                  IMapUpdater& map_updater, IGui& gui ) {
  while( true )
    co_await next_turn( players_state, terrain_state,
                        units_state, settings, map_updater,
                        gui );
}

wait<> run_loaded_game( PlayersState&        players_state,
                        TerrainState const&  terrain_state,
                        UnitsState&          units_state,
                        SettingsState const& settings,
                        IMapUpdater& map_updater, IGui& gui ) {
  return co::erase( co::try_<game_quit_interrupt>( [&] {
    return turn_loop( players_state, terrain_state, units_state,
                      settings, map_updater, gui );
  } ) );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_existing_game( IGui& gui ) {
  lua_reload();
  // Leave this here because it depends on the terrain which,
  // when we eventually move away from global game state, may not
  // exist higher than us in the call stack.
  MapUpdater map_updater(
      GameState::terrain(),
      global_renderer_use_only_when_needed() );
  CHECK_HAS_VALUE( load_game( map_updater, 0 ) );
  reinitialize_planes( map_updater );
  play( e_game_module_tune_points::start_game );
  co_await run_loaded_game(
      GameState::players(), GameState::terrain(),
      GameState::units(), GameState::settings(), map_updater,
      gui );
}

wait<> run_new_game( IGui& gui ) {
  lua_reload();
  default_construct_game_state();
  // Leave this here because it depends on the terrain which,
  // when we eventually move away from global game state, may not
  // exist higher than us in the call stack.
  MapUpdater map_updater(
      GameState::terrain(),
      global_renderer_use_only_when_needed() );
  reinitialize_planes( map_updater );
  lua::state& st = lua_global_state();
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );

  // 1. Take user through game setup/configuration.

  // 2. Generate game world.

  // 3. Game intro sequence in old world view.

  // 4. Animate ship sailing a few squares.

  // 5. Display some messages to the player.

  // 6. Player takes control.
  play( e_game_module_tune_points::start_game );
  co_await run_loaded_game(
      GameState::players(), GameState::terrain(),
      GameState::units(), GameState::settings(), map_updater,
      gui );
}

} // namespace rn
