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
waitable_promise<> g_game_exit_clicked;

/****************************************************************
** Saving / Loading
*****************************************************************/
void main_menu_load() { CHECK_HAS_VALUE( load_game( 0 ) ); }

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
  g_game_exit_clicked.set_value_emplace();
  return false;
};

MENU_ITEM_HANDLER( exit, quit_handler, [] { return true; } );

waitable<> exit_waiter() {
  while( true ) {
    g_game_exit_clicked = waitable_promise<>{};
    co_await g_game_exit_clicked.get_waitable();
    // Game => Exit has been selected.
    // if( !dirty ) co_return;
    auto res = co_await ui::ok_cancel(
        "Really leave game without saving?" );
    if( res == ui::e_ok_cancel::ok ) break;
  }
}

/****************************************************************
** In the Game
*****************************************************************/
waitable<> play_game() {
  conductor::play_request(
      conductor::e_request::fife_drum_happy,
      conductor::e_request_probability::always );
  waitable<> exit_game = exit_waiter();
  auto       next_turn = make_waitable<>();
  while( !exit_game ) {
    // We must clear the callbacks before reusing this object as
    // dictated by the when_any contract.
    exit_game.cancel();
    next_turn = do_next_turn();
    co_await when_any( next_turn, exit_game );
  }
  // If we're here then the exit_game coroutine has finished and
  // been destroyed, so all that we need to is check if we were
  // in the middle of a turn when the exit was done (likely) and
  // destroy that coroutine to avoid leaking it.
  if( next_turn ) next_turn.cancel();
}

/****************************************************************
** New Game
*****************************************************************/
void main_menu_new_game() {
  default_construct_savegame_state();
  // FIXME: temporary, since default constructing the save game
  // state resets the plane state.
  push_plane_config( e_plane_config::main_menu );
  push_plane_config( e_plane_config::terrain );
  lua::reload();
  lua::run_startup_main();
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
        main_menu_new_game();
        co_await play_game();
        break;
      case e_main_menu_item::load:
        main_menu_load();
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
