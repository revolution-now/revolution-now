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
#include "logging.hpp"
#include "lua.hpp"
#include "plane-ctrl.hpp"
#include "save-game.hpp"
#include "turn.hpp"

using namespace std;

namespace rn {

namespace {

waitable<> run_loaded_game() {
  conductor::play_request(
      conductor::e_request::fife_drum_happy,
      conductor::e_request_probability::always );
  try {
    // FIXME: if we just do a `return` here then we won't be able
    // to catch exceptions thrown after a suspension. Instead of
    // wrapping this in a try/catch block, create a combinator
    // that does it somehow.
    co_await co::repeat( next_turn );
  } catch( game_quit_exception const& ) {}
  lg.info( "game exited." );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
waitable<> run_existing_game() {
  CHECK_HAS_VALUE( load_game( 0 ) );
  // Allow all the planes to update their state at least once be-
  // fore we proceed.
  co_await 1_frames;
  co_await run_loaded_game();
}

waitable<> run_new_game() {
  default_construct_savegame_state();
  // FIXME: temporary, since default constructing the save game
  // state resets the plane state.
  push_plane_config( e_plane_config::main_menu );
  push_plane_config( e_plane_config::terrain );
  lua::reload();
  lua::run_startup_main();
  // Allow all the planes to update their state at least once be-
  // fore we proceed.
  co_await 1_frames;

  // 1. Take user through game setup/configuration.

  // 2. Generate game world.

  // 3. Game intro sequence in old world view.

  // 4. Animate ship sailing a few squares.

  // 5. Display some messages to the player.

  // 6. Player takes control.
  co_await run_loaded_game();
}

} // namespace rn
