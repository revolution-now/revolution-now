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
#include "aliases.hpp"
#include "conductor.hpp"
#include "fsm.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "plane-ctrl.hpp"
#include "save-game.hpp"
#include "turn.hpp"
#include "window.hpp"

// Rnl
#include "rnl/app-state.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {

bool g_game_dirty_flag = false;

/****************************************************************
** FSMs
*****************************************************************/
// clang-format off
fsm_transitions( App
 ,(    (main_no_game, new_   ),  ->,  creating
),(    (main_no_game, load   ),  ->,  loading
),(    (main_no_game, quit   ),  ->,  quitting
),(    (main_in_game, leave  ),  ->,  leaving
),(    (main_in_game, save   ),  ->,  saving
),(    (main_in_game, to_game),  ->,  in_game
),(    (creating,     to_game),  ->,  in_game
),(    (leaving,      ok     ),  ->,  main_no_game
),(    (leaving,      cancel ),  ->,  main_in_game
),(    (loading,      to_game),  ->,  in_game
),(    (saving,       to_main),  ->,  main_in_game
),(    (in_game,      to_main),  ->,  main_in_game
) );
// clang-format on

struct AppFsm;
AppFsm& g_app_state();

fsm_class( App ) { //
  fsm_init( AppState::main_no_game{} );

  // FIXME: this should be done in a fsm_on_change_to.
  fsm_transition_( App, leaving, ok, ->, main_no_game ) {
    clear_plane_stack();
    push_plane_config( e_plane_config::main_menu );
    set_main_menu( e_main_menu_type::no_game );
    return {};
  }

  // FIXME: this should be done in a fsm_on_change_to.
  fsm_transition_( App, leaving, cancel, ->, main_in_game ) {
    set_main_menu( e_main_menu_type::in_game );
    return {};
  }

  // FIXME: this should be done in a fsm_on_change_to.
  fsm_transition_( App, saving, to_main, ->, main_in_game ) {
    set_main_menu( e_main_menu_type::in_game );
    return {};
  }

  fsm_transition_( App, in_game, to_main, ->, main_in_game ) {
    push_plane_config( e_plane_config::main_menu );
    set_main_menu( e_main_menu_type::in_game );
    return {};
  }

  fsm_transition_( App, main_no_game, load, ->, loading ) {
    return { /*slot=*/nullopt };
  }

  fsm_transition_( App, main_in_game, save, ->, saving ) {
    return { /*slot=*/nullopt };
  }

  fsm_transition( App, main_in_game, leave, ->, leaving ) {
    (void)cur;
    if( event.dirty ) {
      // FIXME: use sync_futures here.
      ui::ok_cancel(
          "Really leave game without saving?",
          []( ui::e_ok_cancel oc ) {
            if( oc == ui::e_ok_cancel::ok )
              g_app_state().send_event( AppEvent::ok{} );
            else
              g_app_state().send_event( AppEvent::cancel{} );
          } );
      return {};
    } else {
      g_app_state().send_event( AppEvent::ok{} );
      return {};
    }
  }

  // FIXME: this should be done in an on-leave function.
  fsm_transition_( App, main_in_game, to_game, ->, in_game ) {
    pop_plane_config();
    return {};
  }

  // FIXME: this should be done in an on-leave function.
  fsm_transition_( App, creating, to_game, ->, in_game ) {
    clear_plane_stack();
    push_plane_config( e_plane_config::terrain );
    conductor::play_request(
        conductor::e_request::fife_drum_happy,
        conductor::e_request_probability::always );
    return {};
  }

  // FIXME: this should be done in an on-leave function.
  fsm_transition_( App, loading, to_game, ->, in_game ) {
    // FIXME: here we assume that the main menu was at the top of
    // the stack when serialized (check this).
    pop_plane_config();
    conductor::play_request(
        conductor::e_request::fife_drum_happy,
        conductor::e_request_probability::always );
    return {};
  }
};

FSM_DEFINE_FORMAT_RN_( App );

AppFsm& g_app_state() {
  static AppFsm fsm;
  return fsm;
}

// Will be called repeatedly until no more events added to fsm.
void advance_app_state_fsm( AppFsm& fsm, bool* quit ) {
  switch_( fsm.state() ) {
    case_( AppState::main_no_game ) {
      auto sel = main_menu_selection();
      if( !sel.has_value() ) break_;
      switch( *sel ) {
        case e_main_menu_item::resume: //
          SHOULD_NOT_BE_HERE;
          break;
        case e_main_menu_item::new_: //
          fsm.send_event( AppEvent::new_{} );
          break;
        case e_main_menu_item::load: //
          fsm.send_event( AppEvent::load{} );
          break;
        case e_main_menu_item::save: //
          SHOULD_NOT_BE_HERE;
          break;
        case e_main_menu_item::leave: //
          SHOULD_NOT_BE_HERE;
          break;
        case e_main_menu_item::quit: //
          fsm.send_event( AppEvent::quit{} );
          break;
      }
    }
    case_( AppState::main_in_game ) {
      auto sel = main_menu_selection();
      if( !sel.has_value() ) break_;
      switch( *sel ) {
        case e_main_menu_item::resume: //
          fsm.send_event( AppEvent::to_game{} );
          break;
        case e_main_menu_item::new_: //
          SHOULD_NOT_BE_HERE;
          break;
        case e_main_menu_item::load: //
          SHOULD_NOT_BE_HERE;
          break;
        case e_main_menu_item::save: //
          fsm.send_event( AppEvent::save{} );
          break;
        case e_main_menu_item::leave: //
          fsm.send_event(
              AppEvent::leave{ /*dirty=*/g_game_dirty_flag } );
          break;
        case e_main_menu_item::quit: //
          SHOULD_NOT_BE_HERE;
          break;
      }
    }
    case_( AppState::creating ) {
      default_construct_savegame_state();
      lua::reload();
      lua::run_startup_main();
      fsm.send_event( AppEvent::to_game{} );
    }
    case_( AppState::leaving ) {
      //
    }
    case_( AppState::loading, slot ) {
      (void)slot; //
      CHECK_XP( load_game( 0 ) );
      fsm.send_event( AppEvent::to_game() );
    }
    case_( AppState::saving, slot ) {
      (void)slot; //
      CHECK_XP( save_game( 0 ) );
      g_game_dirty_flag = false;
      fsm.send_event( AppEvent::to_main() );
    }
    case_( AppState::in_game ) {
      g_game_dirty_flag = true;
      advance_turn_state(); //
    }
    case_( AppState::quitting ) {
      *quit = true; //
    }
    switch_exhaustive;
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool back_to_main_menu() {
  // FIXME: find a better way to handle this.
  if( g_app_state().holds<AppState::in_game>() &&
      !g_app_state().has_pending_events() ) {
    g_app_state().send_event( AppEvent::to_main{} );
    return true;
  }
  return false;
}

bool advance_app_state() {
  bool quit = false;
  fsm_auto_advance( g_app_state(), "app-state",
                    { advance_app_state_fsm }, &quit );
  return quit;
}

/****************************************************************
** Testing
*****************************************************************/
void test_app_state() {
  //
}

} // namespace rn
