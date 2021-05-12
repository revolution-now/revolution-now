/****************************************************************
**app-ctrl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Handles the top-level game state machines.
*
*****************************************************************/
#include "app-ctrl.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "conductor.hpp"
#include "game.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "plane-ctrl.hpp"
#include "window.hpp" // FIXME

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Main Menu.
*****************************************************************/
waitable<> main_menu() {
  conductor::play_request(
      conductor::e_request::fife_drum_happy,
      conductor::e_request_probability::always );
  while( true ) {
    clear_plane_stack();
    push_plane_config( e_plane_config::main_menu );
    switch( co_await next_main_menu_item() ) {
      case e_main_menu_item::new_:
        co_await run_new_game();
        break;
      case e_main_menu_item::load:
        co_await run_existing_game();
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
