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
#include "gs-root.hpp"
#include "interrupts.hpp"
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

wait<> run_loaded_game(
    Planes& planes, PlayersState& players_state,
    TerrainState const& terrain_state,
    LandViewState& land_view_state, UnitsState& units_state,
    SettingsState const& settings, TurnState& turn_state,
    ColoniesState& colonies_state, IMapUpdater& map_updater ) {
  // TODO: temporary until we have AI.
  bool found_human = false;
  for( auto const& [nation, player] : players_state.players )
    found_human |= player.human;
  CHECK( found_human,
         "there must be at least one human player." );

  return co::erase( co::try_<game_quit_interrupt>( [&] {
    return turn_loop( planes, players_state, terrain_state,
                      land_view_state, units_state, settings,
                      turn_state, colonies_state, map_updater );
  } ) );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_existing_game( Planes& planes ) {
  CHECK_HAS_VALUE( load_game( 0 ) );
  // Leave this here because it depends on the terrain which,
  // when we eventually move away from global game state, may not
  // exist higher than us in the call stack.
  MapUpdater map_updater(
      GameState::terrain(),
      global_renderer_use_only_when_needed() );
  map_updater.just_redraw_map();
  lua_reload( GameState::root() );
  play( e_game_module_tune_points::start_game );
  co_await run_loaded_game(
      planes, GameState::players(), GameState::terrain(),
      GameState::land_view(), GameState::units(),
      GameState::settings(), GameState::turn(),
      GameState::colonies(), map_updater );
}

wait<> run_new_game( Planes& planes ) {
  default_construct_game_state();
  lua_reload( GameState::root() );
  lua::state& st = lua_global_state();
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );
  // Leave this here because it depends on the terrain which,
  // when we eventually move away from global game state, may not
  // exist higher than us in the call stack.
  MapUpdater map_updater(
      GameState::terrain(),
      global_renderer_use_only_when_needed() );
  map_updater.just_redraw_map();

  // 1. Take user through game setup/configuration.

  // 2. Generate game world.

  // 3. Game intro sequence in old world view.

  // 4. Animate ship sailing a few squares.

  // 5. Display some messages to the player.

  // 6. Player takes control.
  play( e_game_module_tune_points::start_game );
  co_await run_loaded_game(
      planes, GameState::players(), GameState::terrain(),
      GameState::land_view(), GameState::units(),
      GameState::settings(), GameState::turn(),
      GameState::colonies(), map_updater );
}

} // namespace rn
