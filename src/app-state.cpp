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
#include "fsm.hpp"
#include "turn.hpp"

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
         ( in_game )             //
);

adt_rn_( AppEvent,    //
         ( new_ ),    //
         ( load ),    //
         ( leave ),   //
         ( save ),    //
         ( to_game ), //
         ( to_main )  //
);

// clang-format off
fsm_transitions( App
 ,(    (main_no_game, new_   ),  ->,  creating
),(    (main_no_game, load   ),  ->,  loading
),(    (main_in_game, leave  ),  ->,  leaving
),(    (main_in_game, save   ),  ->,  saving
),(    (main_in_game, to_game),  ->,  in_game
),(    (creating,     to_game),  ->,  in_game
),(    (leaving,      to_main),  ->,  main_no_game
),(    (loading,      to_game),  ->,  in_game
),(    (saving,       to_main),  ->,  main_in_game
),(    (in_game,      to_main),  ->,  main_in_game
) );
// clang-format on

fsm_class( App ) { //
  fsm_init( AppState::in_game{} );

  fsm_transition_( App, in_game, to_main, ->, main_in_game ) {
    g_game_dirty_flag = true;
    return {};
  }

  fsm_transition_( App, main_no_game, load, ->, loading ) {
    return { /*slot=*/nullopt };
  }

  fsm_transition_( App, main_in_game, save, ->, saving ) {
    return { /*slot=*/nullopt };
  }
};

FSM_DEFINE_FORMAT_RN_( App );

AppFsm g_app_state;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void advance_app_state() {
  switch_( g_app_state.state() ) {
    case_( AppState::main_no_game ) {
      //
    }
    case_( AppState::main_in_game ) {
      //
    }
    case_( AppState::creating ) {
      //
    }
    case_( AppState::leaving ) {
      //
    }
    case_( AppState::loading, slot ) {
      (void)slot; //
    }
    case_( AppState::saving, slot ) {
      (void)slot; //
    }
    case_( AppState::in_game ) {
      //
    }
    switch_exhaustive;
  }
}

/****************************************************************
** Testing
*****************************************************************/
void test_app_state() {
  //
}

} // namespace rn
