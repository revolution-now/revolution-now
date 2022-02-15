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
#include "plane.hpp"
#include "save-game.hpp"
#include "turn.hpp"

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

wait<> turn_loop() {
  while( true ) co_await next_turn();
}

wait<> run_loaded_game() {
  return co::erase( co::try_<game_quit_interrupt>( turn_loop ) );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_existing_game() {
  lua_reload();
  CHECK_HAS_VALUE( load_game( 0 ) );
  reinitialize_planes();
  play( e_game_module_tune_points::start_game );
  co_await run_loaded_game();
}

wait<> run_new_game() {
  lua_reload();
  default_construct_game_state();
  run_lua_startup_main();
  reinitialize_planes();

  // 1. Take user through game setup/configuration.

  // 2. Generate game world.

  // 3. Game intro sequence in old world view.

  // 4. Animate ship sailing a few squares.

  // 5. Display some messages to the player.

  // 6. Player takes control.
  play( e_game_module_tune_points::start_game );
  co_await run_loaded_game();
}

} // namespace rn
