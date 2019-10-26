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
#include "adt.hpp"
#include "aliases.hpp"
#include "conductor.hpp"
#include "fsm.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "main-menu.hpp"
#include "save-game.hpp"
#include "turn.hpp"
#include "window.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {

bool g_game_dirty_flag = false;

/****************************************************************
** FSMs
*****************************************************************/
adt_rn_( AppState,               //
         ( main_no_game ),       //
         ( main_in_game ),       //
         ( creating ),           //
         ( leaving ),            //
         ( loading,              //
           ( Opt<int>, slot ) ), //
         ( saving,               //
           ( Opt<int>, slot ) ), //
         ( in_game ),            //
         ( quitting )            //
);

adt_rn_( AppEvent,            //
         ( ok ),              //
         ( cancel ),          //
         ( new_ ),            //
         ( load ),            //
         ( leave,             //
           ( bool, dirty ) ), //
         ( save ),            //
         ( to_game ),         //
         ( to_main ),         //
         ( quit )             //
);

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
    show_main_menu_plane();
    set_main_menu( e_main_menu_type::no_game );
    return {};
  }

  // FIXME: this should be done in a fsm_on_change_to.
  fsm_transition_( App, leaving, cancel, ->, main_in_game ) {
    // probably not necessary.
    show_main_menu_plane();
    set_main_menu( e_main_menu_type::in_game );
    return {};
  }

  // FIXME: this should be done in a fsm_on_change_to.
  fsm_transition_( App, saving, to_main, ->, main_in_game ) {
    show_main_menu_plane();
    set_main_menu( e_main_menu_type::in_game );
    return {};
  }

  fsm_transition_( App, in_game, to_main, ->, main_in_game ) {
    show_main_menu_plane();
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
    hide_main_menu_plane();
    return {};
  }

  // FIXME: this should be done in an on-leave function.
  fsm_transition_( App, creating, to_game, ->, in_game ) {
    hide_main_menu_plane();
    conductor::play_request(
        conductor::e_request::fife_drum_happy,
        conductor::e_request_probability::always );
    return {};
  }

  // FIXME: this should be done in an on-leave function.
  fsm_transition_( App, loading, to_game, ->, in_game ) {
    hide_main_menu_plane();
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
  bool quit       = false;
  bool had_events = g_app_state().has_pending_events();
  g_app_state().process_events();
  if( had_events ) lg.debug( "app state: {}", g_app_state() );
  switch_( g_app_state().state() ) {
    case_( AppState::main_no_game ) {
      auto sel = main_menu_selection();
      if( !sel.has_value() ) break_;
      switch( *sel ) {
        case +e_main_menu_item::resume: //
          SHOULD_NOT_BE_HERE;
          break;
        case +e_main_menu_item::new_: //
          g_app_state().send_event( AppEvent::new_{} );
          break;
        case +e_main_menu_item::load: //
          g_app_state().send_event( AppEvent::load{} );
          break;
        case +e_main_menu_item::save: //
          SHOULD_NOT_BE_HERE;
          break;
        case +e_main_menu_item::leave: //
          SHOULD_NOT_BE_HERE;
          break;
        case +e_main_menu_item::quit: //
          g_app_state().send_event( AppEvent::quit{} );
          break;
      }
    }
    case_( AppState::main_in_game ) {
      auto sel = main_menu_selection();
      if( !sel.has_value() ) break_;
      switch( *sel ) {
        case +e_main_menu_item::resume: //
          g_app_state().send_event( AppEvent::to_game{} );
          break;
        case +e_main_menu_item::new_: //
          SHOULD_NOT_BE_HERE;
          break;
        case +e_main_menu_item::load: //
          SHOULD_NOT_BE_HERE;
          break;
        case +e_main_menu_item::save: //
          g_app_state().send_event( AppEvent::save{} );
          break;
        case +e_main_menu_item::leave: //
          g_app_state().send_event(
              AppEvent::leave{ /*dirty=*/g_game_dirty_flag } );
          break;
        case +e_main_menu_item::quit: //
          SHOULD_NOT_BE_HERE;
          break;
      }
    }
    case_( AppState::creating ) {
      CHECK_XP( reset_savegame_state() );
      lua::reload();
      lua::run_startup_main();
      g_app_state().send_event( AppEvent::to_game{} );
    }
    case_( AppState::leaving ) {
      //
    }
    case_( AppState::loading, slot ) {
      (void)slot; //
      CHECK_XP( load_game( 0 ) );
      g_app_state().send_event( AppEvent::to_game() );
    }
    case_( AppState::saving, slot ) {
      (void)slot; //
      CHECK_XP( save_game( 0 ) );
      g_game_dirty_flag = false;
      g_app_state().send_event( AppEvent::to_main() );
    }
    case_( AppState::in_game ) {
      g_game_dirty_flag = true;
      advance_turn_state(); //
    }
    case_( AppState::quitting ) {
      quit = true; //
    }
    switch_exhaustive;
  }
  return quit;
}

/****************************************************************
** Testing
*****************************************************************/
void test_app_state() {
  //
}

} // namespace rn
