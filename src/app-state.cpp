/****************************************************************
**app-state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Handles the top-level game state machines.
*
*****************************************************************/
#include "app-state.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "conductor.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "menu.hpp"
#include "plane-ctrl.hpp"
#include "save-game.hpp"
#include "turn.hpp"
#include "waitable-coro.hpp"
#include "window.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Global State
*****************************************************************/
co::ticker g_game_exit;

/****************************************************************
** Saving / Loading
*****************************************************************/
waitable<> main_menu_load() {
  CHECK_HAS_VALUE( load_game( 0 ) );
  // Allow all the planes to update their state at least once be-
  // fore we proceed.
  co_await 1_frames;
}

waitable<> menu_save_handler() {
  if( auto res = save_game( 0 ); !res ) {
    co_await ui::message_box(
        "There was a problem saving the game." );
    lg.error( "failed to save game: {}", res.error() );
  } else {
    co_await ui::message_box(
        fmt::format( "Successfully saved game to {}.", res ) );
    lg.info( "saved game to {}.", res );
  }
}

MENU_ITEM_HANDLER(
    save, []() -> waitable<> { co_await menu_save_handler(); },
    [] { return true; } );

/****************************************************************
** Exiting
*****************************************************************/
function<bool( void )> quit_handler = [] {
  g_game_exit.tick();
  return false;
};

MENU_ITEM_HANDLER( exit, quit_handler, [] { return true; } );

waitable<> exit_waiter() {
  return g_game_exit.wait();
  // while( true ) {
  //   co_await g_game_exit.wait();
  //   // Game => Exit has been selected.
  //   // if( !dirty ) co_return;
  //   auto res = co_await ui::ok_cancel(
  //       "Really leave game without saving?" );
  //   if( res == ui::e_ok_cancel::ok ) break;
  // }
}

/****************************************************************
** In the Game
*****************************************************************/
waitable<> play_game() {
  conductor::play_request(
      conductor::e_request::fife_drum_happy,
      conductor::e_request_probability::always );
  return co::any( co::repeat( do_next_turn ), exit_waiter() );
}

/****************************************************************
** New Game
*****************************************************************/
waitable<> main_menu_new_game() {
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
}

/****************************************************************
** Main Menu
*****************************************************************/
waitable<> main_menu() {
  while( true ) {
    clear_plane_stack();
    push_plane_config( e_plane_config::main_menu );
    switch( co_await next_main_menu_item() ) {
      case e_main_menu_item::new_:
        co_await main_menu_new_game();
        co_await play_game();
        break;
      case e_main_menu_item::load:
        co_await main_menu_load();
        co_await play_game();
        break;
      case e_main_menu_item::quit: //
        co_return;
      case e_main_menu_item::settings_graphics:
        co_await ui::message_box( "No graphics settings yet." );
        break;
      case e_main_menu_item::settings_sound:
        co_await ui::message_box( "No sound settings yet." );
        break;
    }
  }
}

} // namespace

/****************************************************************
** Top-Level Application Flow.
*****************************************************************/
waitable<> revolution_now() { co_await main_menu(); }

} // namespace rn
