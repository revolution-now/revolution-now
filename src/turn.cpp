/****************************************************************
**turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#include "turn.hpp"

// rds
#include "turn-impl.rds.hpp"

// Revolution Now
#include "auto-save.hpp"
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "colonies-turn.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "command.hpp"
#include "continental.hpp"
#include "declare.hpp"
#include "disband.hpp"
#include "fathers.hpp"
#include "game-options.hpp"
#include "harbor-units.hpp"
#include "harbor-view.hpp"
#include "icolony-evolve.rds.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "interrupts.hpp"
#include "intervention.hpp"
#include "iraid.rds.hpp"
#include "isave-game.rds.hpp"
#include "itribe-evolve.rds.hpp"
#include "iuser-config.hpp"
#include "land-view.hpp"
#include "map-edit.hpp"
#include "market.hpp"
#include "minds.hpp"
#include "native-turn.hpp"
#include "on-map.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "plow.hpp"
#include "rcl-game-storage.hpp"
#include "rebel-sentiment.hpp"
#include "ref.hpp"
#include "report-congress.hpp"
#include "road.hpp"
#include "roles.hpp"
#include "save-game.hpp"
#include "succession.hpp"
#include "tax.hpp"
#include "ts.hpp"
#include "turn-mgr.hpp"
#include "turn-plane.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "unsentry.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/nation.hpp"
#include "config/turn.rds.hpp"
#include "config/unit-type.rds.hpp"
#include "config/user.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/nation.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/logger.hpp"
#include "base/scope-exit.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"
#include "base/variant-util.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <algorithm>
#include <deque>
#include <queue>

using namespace std;

namespace rn {

namespace {

using ::base::NoDiscard;
using ::gfx::point;

/****************************************************************
** Global State
*****************************************************************/
// Globals relevant to end of turn.
struct next_turn_t {};

using UserInputEndOfTurn = base::variant< //
    e_menu_item,                          //
    LandViewPlayerInput,                  //
    next_turn_t                           //
    >;

/****************************************************************
** Helpers
*****************************************************************/
// To be called once per turn.
wait<> advance_time( IGui& gui, TurnTimePoint& time_point ) {
  ++time_point.turns;
  if( time_point.year == 1600 &&
      time_point.season == e_season::spring )
    co_await gui.message_box(
        "Starting in the year [1600] the time scale "
        "changes.  Henceforth there will be both a "
        "[Spring] and a [Fall] turn each year." );
  bool const two_turns = ( time_point.year >= 1600 );
  switch( time_point.season ) {
    case e_season::winter:
      // We're not currently supporting four seasons per year, so
      // just revert it to the spring/fall cycle.
      time_point.season = e_season::spring;
      break;
    case e_season::spring:
      if( two_turns ) {
        // Two seasons per year.
        time_point.season = e_season::autumn;
      } else {
        // Stay in Spring and just go to the next year.
        ++time_point.year;
      }
      break;
    case e_season::summer:
      // We're not currently supporting four seasons per year, so
      // just revert it to the spring/fall cycle.
      time_point.season = e_season::autumn;
      break;
    case e_season::autumn:
      // Two seasons per year.
      time_point.season = e_season::spring;
      ++time_point.year;
      break;
  }
}

// This is called when the user tries to do something that might
// leave the current game. It will check if the current game has
// unsaved changes and, if so, will ask the user if they want to
// save it. If the user does, then it will open the save-game di-
// alog box. If the user escapes out of that dialog box (i.e.,
// does not explicitly select "no") then this function will re-
// turn false, since that can be useful to the caller, since it
// may indicate that the user is trying to abort whatever process
// prompted the save-game box to appear in the first place.
//
// Will return whether it is safe to proceed.
wait<NoDiscard<bool>> check_if_not_dirty_or_can_proceed(
    IEngine& engine, SSConst const& ss, TS& ts,
    IGameStorageSave const& storage_save ) {
  auto is_game_saved = [&] {
    // Checks if the serializable game state has been modified in
    // any way since the last time it was saved or loaded.
    return base::timer( "saved state comparison", [&] {
      return root_states_equal( ss.root, ts.saved );
    } );
  };
  if( is_game_saved() ) co_return true;
  if( !engine.user_config()
           .read()
           .game_saving.ask_need_save_when_leaving )
    co_return true;
  YesNoConfig const config{ .msg =
                                "This game has unsaved changes. "
                                "Would you like to save?",
                            .yes_label      = "Yes",
                            .no_label       = "No",
                            .no_comes_first = false };

  maybe<ui::e_confirm> const answer =
      co_await ts.gui.optional_yes_no( config );
  if( !answer.has_value() ) co_return false;
  if( answer == ui::e_confirm::no ) co_return true;
  maybe<int> const slot =
      co_await select_save_slot( engine, ts, storage_save );
  if( !slot.has_value() ) co_return false;
  RealGameSaver const game_saver( ss, ts, storage_save );
  bool const saved =
      co_await game_saver.save_to_slot_interactive( *slot );
  co_return saved;
}

wait<> proceed_to_exit( IEngine& engine, SSConst const& ss,
                        TS& ts ) {
  YesNoConfig const config{ .msg            = "Exit to DOS?",
                            .yes_label      = "Yes",
                            .no_label       = "No",
                            .no_comes_first = true };
  maybe<ui::e_confirm> const answer =
      co_await ts.gui.optional_yes_no( config );
  if( answer != ui::e_confirm::yes ) co_return;
  // TODO: we may want to inject these somewhere higher up.
  RclGameStorageSave const storage_save( ss );
  bool const can_proceed =
      co_await check_if_not_dirty_or_can_proceed( engine, ss, ts,
                                                  storage_save );
  if( can_proceed ) throw game_quit_interrupt{};
}

// If the element is present in the deque then it will be erased.
// The function will return true if the value was found (and
// erased). This will erase multiple instances of the element if
// there are multiple in the queue. Note that this is order N.
bool erase_from_deque_if_present( deque<UnitId>& q, UnitId id ) {
  bool found = false;
  while( true ) {
    auto it = find( q.begin(), q.end(), id );
    if( it == q.end() ) break;
    found = true;
    q.erase( it );
  }
  return found;
}

void prioritize_unit( deque<UnitId>& q, UnitId id ) {
  erase_from_deque_if_present( q, id );
  q.push_front( id );
}

// We use movement points for all units to track whether they've
// been advanced this turn, even for those that are not on the
// map. That is so that we don't have to introduce another piece
// of state to track that.
void finish_turn( Unit& unit ) { unit.forfeight_mv_points(); }

[[nodiscard]] bool finished_turn( Unit const& unit ) {
  return unit.mv_pts_exhausted();
}

bool should_remove_unit_from_queue( Unit const& unit ) {
  if( finished_turn( unit ) ) return true;
  switch( unit.orders().to_enum() ) {
    using e = unit_orders::e;
    case e::fortified:
      return true;
    case e::fortifying:
      return false;
    case e::sentry:
      return true;
    case e::road:
      return false;
    case e::plow:
      return false;
    case e::none:
      return false;
    case e::damaged:
      return false;
  }
}

vector<UnitId> euro_units_all( UnitsState const& units_state,
                               e_player n ) {
  vector<UnitId> res;
  res.reserve( units_state.all().size() );
  for( auto const& p : units_state.euro_all() )
    if( n == p.second->unit.player_type() )
      res.push_back( p.first );
  return res;
}

// Apply a function to all european units. The function may mu-
// tate the units.
void map_all_euro_units(
    UnitsState& units_state,
    base::function_ref<void( Unit& )> func ) {
  for( auto& p : units_state.euro_all() )
    func( units_state.unit_for( p.first ) );
}

bool is_unit_on_high_seas( SSConst const& ss,
                           UnitId const unit_id ) {
  return is_unit_inbound( ss.units, unit_id ) ||
         is_unit_outbound( ss.units, unit_id );
}

void remove_deleted_units( SSConst const& ss,
                           deque<UnitId>& q ) {
  q.erase( remove_if( q.begin(), q.end(),
                      [&]( UnitId const unit_id ) {
                        return !ss.units.exists( unit_id );
                      } ),
           q.end() );
}

// This won't actually prioritize the units, it will just check
// the request and return the units that can be prioritized.
wait<vector<UnitId>> process_unit_prioritization_request(
    SSConst const& ss, TS& ts,
    LandViewPlayerInput::prioritize const& request ) {
  // Move some units to the front of the queue.
  auto prioritize = request.units;
  // The "prioritize" action should only ever be used when we
  // have at least one unit to prioritize. If we don't then the
  // "activate" action should be used.
  CHECK( !prioritize.empty() );
  erase_if( prioritize, [&]( UnitId id ) {
    return finished_turn( ss.units.unit_for( id ) );
  } );
  auto orig_size = request.units.size();
  auto curr_size = prioritize.size();
  CHECK( curr_size <= orig_size );
  if( curr_size == 0 )
    co_await ts.gui.message_box(
        "The selected unit(s) have already moved this turn." );
  else if( curr_size < orig_size )
    co_await ts.gui.message_box(
        "Some of the selected units have already moved this "
        "turn." );
  co_return prioritize;
}

wait<NoDiscard<bool>> process_unit_prioritization_request(
    SSConst const& ss, TS& ts, UnitId const unit_id ) {
  LandViewPlayerInput::prioritize const request{
    .units = { unit_id } };
  vector<UnitId> const res =
      co_await process_unit_prioritization_request( ss, ts,
                                                    request );
  if( res.empty() ) co_return false;
  CHECK( res.size() == 1 );
  co_return true;
}

// It might seem to make sense to just autosave the game at the
// start of each turn or at least at the start of the player's
// turn. However, like in the OG, we instead autosave the game
// just before the player is asked for input (i.e., after the na-
// tives have moved, after any other european nations before
// them, and after the player's colonies have evolved). That way,
// when they load that file they will see a unit asking for or-
// ders as opposed to natives immediately moving, or colony mes-
// sages popping up, which should make for a better experience.
// If there are no units that ask for orders in a turn, then the
// autosave will happen on the end of turn prompt when the player
// is being asked for input to move the white box.
//
// Since the player is guaranteed to have at least one of the
// following each turn: 1) a blinking unit asking for input, or
// 2) the end-of-turn sign blinking waiting for input, it is suf-
// ficient to attempt to autosave at those two points. Since
// sometimes both can happen (if end-of-turn is enabled in game
// settings), the autosave mechanism makes sure never to save
// twice per turn. Note that there is also "view mode" which the
// user can enter during a turn to inspect using the white
// square. However, said mode cannot be entered unless there is
// first a unit asking for orders, so this case is covered by au-
// tosaving when a unit asks for orders.
//
// All of that said, note that the OG has a bug in game loading
// that can cause confusion. There is a flag in the OG's SAV file
// (that we've labeled manual_save_flag) that indicates whether a
// sav file was produced manually or via autosave. When it is an
// autosave file, and it is loaded just after the game program is
// started, then things are fine. However, if that autosave file
// is loaded after another game was already loaded, then it will
// glitch and it will have the natives and other european nations
// redo their turns, then do another autosave, before ending up
// asking the player for input. Thus, if you just keep loading an
// autosave file, you will see the year steadily increase just
// from the loading. We of course are not reproducing that bug.
void autosave_if_needed( SS& ss, TS& ts ) {
  set<int> const autosave_slots = should_autosave( ss.as_const );
  if( autosave_slots.empty() ) return;
  // TODO: we may want to inject these somewhere higher up.
  RclGameStorageSave const storage_save( ss );
  RealGameSaver const game_saver( ss, ts, storage_save );
  // This will do the save.
  expect<std::vector<fs::path>> const paths_saved =
      autosave( ss.as_const, game_saver, ss.turn.autosave,
                autosave_slots );
  if( !paths_saved.has_value() )
    lg.error( "failed to auto-save: {}", paths_saved.error() );
}

// This is a bit costly, and so it is unfortunate that, for vi-
// sual consistency, we have to call it at the start of each na-
// tion's turn (and the start of the natives' turn). Actually, it
// doesn't even work perfectly, because e.g. if player A de-
// stroy's player B's unit during player A's turn, player B will
// continue to have visibility in the unit's surroundings for the
// remainder of player A's turn. Maybe in the future we can im-
// prove this mechanism by using a more incremental approach,
// e.g. adding fog whenever a unit leaves the map, which we
// should be able to hook into via the change_to_free method in
// the unit-ownership module. Until then, we'll just call this
// for all nations at the start and end of each turn, which for
// the most part does the right thing despite being unnecessarily
// costly. Actually, the cost of it may need to be reassessed;
// for normal game map sizes with normal numbers of units on the
// map, it actually may not be that bad at all.
void recompute_fog_for_all_players( SS& ss, TS& ts ) {
  for( e_player const player : refl::enum_values<e_player> )
    if( ss.players.players[player].has_value() )
      recompute_fog_for_player( ss, ts, player );
}

wait<> declare( SS& ss, TS& ts, Player& player ) {
  co_await declare_independence_ui_sequence_pre(
      ss.as_const, ts, as_const( player ) );
  DeclarationResult const decl_res =
      declare_independence( ss, ts, player );
  co_await declare_independence_ui_sequence_post(
      ss.as_const, ts, as_const( player ), decl_res );
}

/****************************************************************
** Common Player Input Handling.
*****************************************************************/
wait<> open_colony( TS& ts,
                    LandViewPlayerInput::colony const& colony ) {
  e_colony_abandoned const abandoned =
      co_await ts.colony_viewer.show( ts, colony.id );
  if( abandoned == e_colony_abandoned::yes )
    // Nothing really special to do here.
    co_return;
}

wait<> show_hidden_terrain( TS& ts ) {
  co_await ts.planes.get()
      .get_bottom<ILandViewPlane>()
      .show_hidden_terrain();
}

wait<> prioritize_units_during_turn(
    SSConst const& ss, TS& ts, PlayerTurnState::units& nat_units,
    LandViewPlayerInput::prioritize const& prioritize ) {
  vector<UnitId> const units =
      co_await process_unit_prioritization_request( ss, ts,
                                                    prioritize );
  for( UnitId const id_to_add : units )
    prioritize_unit( nat_units.q, id_to_add );
}

wait<> disband_at_location( IEngine& engine, SS& ss, TS& ts,
                            Player const& player,
                            point const tile ) {
  auto const viz = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  if( viz->visible( tile ) == e_tile_visibility::hidden )
    co_return;
  auto const entities =
      disbandable_entities_on_tile( ss.as_const, *viz, tile );
  auto const selected = co_await disband_tile_ui_interaction(
      ss.as_const, ts, engine.textometer(), player, *viz,
      entities );
  co_await execute_disband( ss, ts, *viz, tile, selected );
}

/****************************************************************
** Menu Handlers
*****************************************************************/
wait<> menu_handler( IEngine& engine, SS& ss, TS& ts,
                     Player& player, e_menu_item item ) {
  switch( item ) {
    case e_menu_item::exit: {
      co_await proceed_to_exit( engine, ss, ts );
      break;
    }
    case e_menu_item::save: {
      // TODO: we may want to inject these somewhere higher up.
      RclGameStorageSave const storage_save( ss );
      RealGameSaver const game_saver( ss, ts, storage_save );
      maybe<int> const slot =
          co_await select_save_slot( engine, ts, storage_save );
      if( slot.has_value() ) {
        bool const saved =
            co_await game_saver.save_to_slot_interactive(
                *slot );
        (void)saved;
      }
      break;
    }
    case e_menu_item::load: {
      // TODO: we may want to inject these somewhere higher up.
      RclGameStorageSave const storage_save( ss );
      bool const can_proceed =
          co_await check_if_not_dirty_or_can_proceed(
              engine, ss, ts, storage_save );
      if( !can_proceed ) break;
      game_load_interrupt load;
      load.slot = co_await select_load_slot( ts, storage_save );
      if( load.slot.has_value() ) throw load;
      break;
    }
    case e_menu_item::revolution: {
      valid_or<e_declare_rejection> const can_declare =
          can_declare_independence( ss, player );
      if( !can_declare ) {
        co_await show_declare_rejection_msg(
            ts.gui, can_declare.error() );
        break;
      }
      ui::e_confirm const answer =
          co_await ask_declare( ts.gui, player );
      if( answer != ui::e_confirm::yes ) break;
      co_await declare( ss, ts, player );
      break;
    }
    case e_menu_item::harbor_view: {
      HarborViewer harbor_viewer( engine, ss, ts, player );
      co_await harbor_viewer.show();
      break;
    }
    case e_menu_item::continental_congress: {
      co_await show_continental_congress_report(
          engine, ss, player, ts.planes );
      break;
    }
    case e_menu_item::cheat_explore_entire_map: {
      cheat_explore_entire_map( ss, ts );
      break;
    }
    case e_menu_item::cheat_set_human_players: {
      co_await cheat_set_human_players( ss, ts );
      break;
    }
    case e_menu_item::cheat_kill_natives: {
      co_await kill_natives( ss, ts );
      break;
    }
    case e_menu_item::cheat_map_editor: {
      // Need to co_await so that the map_updater stays alive.
      co_await run_map_editor( engine, ss, ts );
      break;
    }
    case e_menu_item::cheat_edit_fathers: {
      co_await cheat_edit_fathers( engine, ss, ts, player );
      break;
    }
    case e_menu_item::cheat_advance_revolution_status: {
      co_await cheat_advance_revolution_status( ss, ts, player );
      break;
    }
    case e_menu_item::game_options: {
      co_await open_game_options_box( ss, ts );
      break;
    }
    default: {
      FATAL( "Not supposed to be handling menu item {} here.",
             item );
    }
  }
}

wait<e_menu_item> wait_for_menu_selection(
    IMenuServer& menu_server ) {
  TurnPlane turn_plane( menu_server );
  co_return co_await turn_plane.next_menu_action();
}

/****************************************************************
** Processing Player Input (End of Turn).
*****************************************************************/
wait<EndOfTurnResult> process_player_input_eot(
    IEngine& engine, e_menu_item item, SS& ss, TS& ts,
    Player& player ) {
  // In the future we might need to put logic here that is spe-
  // cific to the end-of-turn, but for now this is sufficient.
  co_await menu_handler( engine, ss, ts, player, item );
  co_return EndOfTurnResult::not_done_yet{};
}

wait<EndOfTurnResult> process_player_input_eot(
    IEngine& engine, LandViewPlayerInput const& input, SS& ss,
    TS& ts, Player& player ) {
  SWITCH( input ) {
    CASE( colony ) {
      co_await open_colony( ts, colony );
      break;
    }
    CASE( european_status ) {
      HarborViewer harbor_viewer( engine, ss, ts, player );
      co_await harbor_viewer.show();
      break;
    }
    CASE( hidden_terrain ) {
      co_await show_hidden_terrain( ts );
      break;
    }
    CASE( view_mode ) {
      // End-of-turn mode is already a view mode, so we should
      // not be getting this command here.
      SHOULD_NOT_BE_HERE;
    }
    CASE( move_mode ) {
      co_return EndOfTurnResult::return_to_units{};
    }
    CASE( next_turn ) { co_return EndOfTurnResult::proceed{}; }
    CASE( exit ) {
      co_await proceed_to_exit( engine, ss, ts );
      break;
    }
    CASE( give_command ) {
      SWITCH( give_command.cmd ) {
        CASE( disband ) {
          CHECK( disband.tile.has_value() );
          co_await disband_at_location( engine, ss, ts, player,
                                        *disband.tile );
          break;
        }
        default:
          break;
      }
      break;
    }
    CASE( activate ) {
      bool const can_prioritize =
          co_await process_unit_prioritization_request(
              ss, ts, activate.unit );
      if( can_prioritize )
        co_return EndOfTurnResult::return_to_units{
          .first_to_ask = activate.unit };
      else
        co_return EndOfTurnResult::not_done_yet{};
    }
    CASE( prioritize ) {
      CHECK( !prioritize.units.empty() );
      vector<UnitId> const units =
          co_await process_unit_prioritization_request(
              ss, ts, prioritize );
      // Unlike during the turn, we don't actually prioritize the
      // list of units that are returned by this function here
      // because there is no active unit queue. So instead we
      // will just let them be as they are (they will have had
      // their orders cleared) and we will just signal to the
      // caller that we want to go back to the units turn, at
      // which point the units with cleared orders will be found
      // (assuming they have movement points left) and they will
      // ask for orders. That said, we do specify the unit that
      // was most recently activated so that it can be communi-
      // cated back to the units phase and that unit can ask for
      // orders first. Otherwise, when the player clears the or-
      // ders of multiple units and then activates the final one
      // so that they start asking for orders, a "random" one
      // would be selected to actually ask for orders first and
      // not the one that the player last clicked on. This would
      // seem strange to the player, and this avoids that.
      EndOfTurnResult::return_to_units ret;
      if( !units.empty() ) ret.first_to_ask = units.back();
      co_return ret;
    }
  }
  co_return EndOfTurnResult::not_done_yet{};
}

wait<EndOfTurnResult> process_player_input_eot( IEngine&,
                                                next_turn_t, SS&,
                                                TS&, Player& ) {
  lg.debug( "end of turn button clicked." );
  co_return EndOfTurnResult::proceed{};
}

wait<EndOfTurnResult> process_input_eot( IEngine& engine, SS& ss,
                                         TS& ts,
                                         Player& player ) {
  auto wait_for_button = co::fmap(
      [] λ( next_turn_t{} ), ts.planes.get()
                                 .panel.typed()
                                 .wait_for_eot_button_click() );
  // The reason that we want to use co::first here instead of in-
  // terleaving the three streams is because as soon as one be-
  // comes ready (and we start processing it) we want all the
  // others to be automatically be cancelled, which will have the
  // effect of disabling further input on them (e.g., disabling
  // menu items), which is what we want for a good user experi-
  // ence.
  UserInputEndOfTurn const command = co_await co::first( //
      wait_for_menu_selection( ts.planes.get().menu ),   //
      ts.planes.get()
          .get_bottom<ILandViewPlane>()
          .eot_get_next_input(),   //
      std::move( wait_for_button ) //
  );
  co_return co_await visit(
      command, LC( process_player_input_eot( engine, _, ss, ts,
                                             player ) ) );
}

// Enters the EOT phase and processes a single input then re-
// turns.
wait<EndOfTurnResult> end_of_turn( IEngine& engine, SS& ss,
                                   TS& ts, Player& player ) {
  // See comments above the autosave_if_needed function for why
  // we are putting this here and how it works.
  autosave_if_needed( ss, ts );
  co_return co_await process_input_eot( engine, ss, ts, player );
}

/****************************************************************
** Processing Player Input (During Turn).
*****************************************************************/
wait<> process_player_input_normal_mode(
    IEngine& engine, UnitId, e_menu_item item, SS& ss, TS& ts,
    Player& player, PlayerTurnState::units& ) {
  // In the future we might need to put logic here that is spe-
  // cific to the mid-turn scenario, but for now this is suffi-
  // cient.
  co_await menu_handler( engine, ss, ts, player, item );
}

wait<> process_player_input_normal_mode(
    IEngine& engine, UnitId id, LandViewPlayerInput const& input,
    SS& ss, TS& ts, Player& player,
    PlayerTurnState::units& nat_units ) {
  auto& st = nat_units;
  auto& q  = st.q;
  SWITCH( input ) {
    CASE( next_turn ) {
      // The land view should never send us a 'next turn' command
      // when we are not at the end of a turn.
      SHOULD_NOT_BE_HERE;
    }
    CASE( exit ) {
      co_await proceed_to_exit( engine, ss, ts );
      break;
    }
    CASE( colony ) {
      co_await open_colony( ts, colony );
      break;
    }
    CASE( european_status ) {
      HarborViewer harbor_viewer( engine, ss, ts, player );
      co_await harbor_viewer.show();
      break;
    }
    CASE( hidden_terrain ) {
      co_await show_hidden_terrain( ts );
      break;
    }
    CASE( view_mode ) {
      view_mode_interrupt e;
      e.options = view_mode.options;
      throw e;
    }
    CASE( move_mode ) { break; }
    // We have some orders for the current unit.
    CASE( give_command ) {
      auto& cmd = give_command.cmd;
      if( cmd.holds<command::wait>() ) {
        // Just remove it form the queue, and it'll get picked up
        // in the next iteration. We don't want to push this unit
        // onto the back of the queue since that will cause it to
        // appear multiple times in the queue, which is actually
        // OK, but not ideal from a UX perspective because it
        // will cause the unit to ask for orders multiple times
        // during a single cycle of "wait" commands, i.e., the
        // user just pressing "wait" through all of the available
        // units. FIXME: this may still not be working ideally,
        // since it can still theoretically cause the same unit
        // to ask for orders twice in a row, if for example some
        // units were prioritized (effectively rearranging the
        // order of the queue, then the player hits wait a few
        // times, then the last unit to ask for orders happens to
        // be the first in the queue on the next round. I think
        // this will probably only happen when there are only a
        // few active units, but it is quite noticeable in that
        // situation and would be nice to fix if possible (though
        // it may be nontrivial) since it causes a weird user ex-
        // perience when it happens.
        CHECK( q.front() == id );
        q.pop_front();
        break;
      }
      if( cmd.holds<command::forfeight>() ) {
        ss.units.unit_for( id ).forfeight_mv_points();
        break;
      }

      co_await ts.planes.get()
          .get_bottom<ILandViewPlane>()
          .ensure_visible_unit( id );
      unique_ptr<CommandHandler> handler =
          command_handler( engine, ss, ts, player, id, cmd );
      CHECK( handler );

      auto run_result = co_await handler->run();
      if( !run_result.order_was_run ) break;

      // !! The unit may no longer exist at this point, e.g. if
      // they were disbanded or if they lost a battle to the na-
      // tives.

      for( auto id : run_result.units_to_prioritize )
        prioritize_unit( q, id );
      break;
    }
    CASE( activate ) {
      // This action is only relevant in view mode and eot mode,
      // i.e. when there isn't already a unit asking for orders.
      SHOULD_NOT_BE_HERE;
    }
    CASE( prioritize ) {
      co_await prioritize_units_during_turn( ss, ts, nat_units,
                                             prioritize );
      break;
    }
  }
}

wait<LandViewPlayerInput> landview_player_input(
    SS& ss, TS& ts, PlayerTurnState::units& nat_units,
    UnitId id ) {
  LandViewPlayerInput response;
  if( auto maybe_command = pop_unit_command( id ) ) {
    response = LandViewPlayerInput::give_command{
      .cmd = *maybe_command };
  } else {
    lg.debug( "asking orders for: {}",
              debug_string( as_const( ss.units ), id ) );
    nat_units.skip_eot = true;
    // See comments above the autosave_if_needed function for why
    // we are putting it in this general location and how it
    // works (note it appears in other locations as well). That
    // said, as a comment about this specific location, we should
    // save the game after we set skip_eot to true otherwise if
    // the player loads an auto-save file that was saved here and
    // then immediately tries to exit the game, the game state
    // will be dirty because the skip_eot will have changed. It
    // won't cause any issues that we're saving with skip_eot set
    // to true because when the auto-save is reloaded from this
    // point, the unit that is asking for orders will still be
    // asking for orders, so skip_eot will again be set to true.
    autosave_if_needed( ss, ts );
    response = co_await ts.planes.get()
                   .get_bottom<ILandViewPlane>()
                   .get_next_input( id );
  }
  co_return response;
}

wait<> query_unit_input( IEngine& engine, UnitId id, SS& ss,
                         TS& ts, Player& player,
                         PlayerTurnState::units& nat_units ) {
  auto command = co_await co::first(
      wait_for_menu_selection( ts.planes.get().menu ),
      landview_player_input( ss, ts, nat_units, id ) );
  co_await visit( command, [&]( auto const& action ) -> wait<> {
    co_await process_player_input_normal_mode(
        engine, id, action, ss, ts, player, nat_units );
  } );
  // A this point we should return because we want to in general
  // allow for the possibility and any action executed above
  // might affect the status of the unit asking for orders, and
  // so returning will cause the unit to be re-examined.
}

/****************************************************************
** View Mode.
*****************************************************************/
wait<> process_player_input_view_mode( IEngine& engine, SS& ss,
                                       TS& ts, Player& player,
                                       PlayerTurnState::units&,
                                       e_menu_item const item ) {
  // In the future we might need to put logic here that is spe-
  // cific to view mode, but for now this is sufficient.
  co_await menu_handler( engine, ss, ts, player, item );
}

wait<> process_player_input_view_mode(
    IEngine& engine, SS& ss, TS& ts, Player& player,
    PlayerTurnState::units& nat_units,
    LandViewPlayerInput const& input ) {
  SWITCH( input ) {
    CASE( view_mode ) { break; }
    CASE( move_mode ) { break; }
    CASE( colony ) {
      co_await open_colony( ts, colony );
      break;
    }
    CASE( european_status ) {
      HarborViewer harbor_viewer( engine, ss, ts, player );
      co_await harbor_viewer.show();
      break;
    }
    CASE( exit ) {
      co_await proceed_to_exit( engine, ss, ts );
      break;
    }
    CASE( hidden_terrain ) {
      co_await show_hidden_terrain( ts );
      break;
    }
    CASE( next_turn ) {
      // The land view should never send us a 'next turn' command
      // when we are not at the end of a turn.
      SHOULD_NOT_BE_HERE;
    }
    CASE( give_command ) {
      SWITCH( give_command.cmd ) {
        CASE( disband ) {
          CHECK( disband.tile.has_value() );
          co_await disband_at_location( engine, ss, ts, player,
                                        *disband.tile );
          break;
        }
        default:
          break;
      }
      break;
    }
    CASE( activate ) {
      bool const can_prioritize =
          co_await process_unit_prioritization_request(
              ss, ts, activate.unit );
      if( !can_prioritize ) break;
      prioritize_unit( nat_units.q, activate.unit );
      break;
    }
    CASE( prioritize ) {
      co_await prioritize_units_during_turn( ss, ts, nat_units,
                                             prioritize );
      break;
    }
  }
  co_return;
}

wait<> show_view_mode( IEngine& engine, SS& ss, TS& ts,
                       Player& player,
                       PlayerTurnState::units& nat_units,
                       ViewModeOptions options ) {
  lg.info( "entering view mode." );
  SCOPE_EXIT { lg.info( "leaving view mode." ); };
  nat_units.view_mode = true;
  SCOPE_EXIT { nat_units.view_mode = false; };
  while( true ) {
    auto const command = co_await co::first(
        wait_for_menu_selection( ts.planes.get().menu ),
        ts.planes.get()
            .get_bottom<ILandViewPlane>()
            .show_view_mode( options ) );
    co_await visit( command, [&]( auto const& action ) {
      return process_player_input_view_mode(
          engine, ss, ts, player, nat_units, action );
    } );
    using I = LandViewPlayerInput;
    bool const leave =
        command.get_if<I>().holds<I::move_mode>() ||
        command.get_if<I>().holds<I::prioritize>() ||
        command.get_if<I>().holds<I::activate>() || //
        false;
    if( leave ) co_return;
    // If we entered view mode targeting a specific tile then we
    // should get rid of it at this point because the player may
    // have moved the white box around while in view mode the
    // first time and so when we circle around and re-enter view
    // mode we don't want the white square to jump back to the
    // initial target tile, which it would do if we keep it in
    // the options argument.
    options.initial_tile = nothing;
  }
}

/****************************************************************
** Advancing Units.
*****************************************************************/
// Returns true if the unit needs to ask the user for input.
wait<bool> advance_unit( IEngine& engine, SS& ss, TS& ts,
                         Player& player, UnitId id ) {
  Unit& unit = ss.units.unit_for( id );
  CHECK( !should_remove_unit_from_queue( unit ) );
  CHECK( !is_unit_on_high_seas( ss, id ),
         "units on the high seas are advanced elsewhere." );

  if( unit.orders().holds<unit_orders::fortifying>() ) {
    // Any units that are in the "fortifying" state at the start
    // of their turn get "promoted" for the "fortified" status,
    // which means they are actually fortified and get those ben-
    // efits. The OG consumes movement points both at the moment
    // that the player initially fortifies the unit, then once on
    // the following turn (where we are now) where the unit is
    // transitioned from "fortifying" to "fortified." But note
    // that thereafter, the fortified unit will not have its
    // movement points consumed at the start of their turn; this
    // allows the player to wake a fortified unit and move it
    // that turn. It's just that the player can't do that for the
    // first two turns, unless they happen to activate the unit
    // while it is in the "fortifying" state but before we do
    // this next step of transitioning it to "fortified" (this is
    // also the behavior of the OG).
    unit.orders() = unit_orders::fortified{};
    unit.forfeight_mv_points();
    co_return false;
  }

  if( auto damaged =
          unit.orders().get_if<unit_orders::damaged>();
      damaged.has_value() ) {
    if( --damaged->turns_until_repair == 0 ) {
      unit.clear_orders();
      if( ss.units.maybe_coord_for( unit.id() ) )
        // Unit is in a colony being repaired, so we can just let
        // it ask for orders.
        co_return true;
      else {
        // Unit is in the harbor. We could make it sail back to
        // the new world, but probably best to just let the
        // player decide.
        co_await ts.gui.message_box(
            "Our [{}] has finished its repairs in [{}].",
            unit.desc().name,
            nation_obj( player.nation ).harbor_city_name );
        HarborViewer harbor_viewer( engine, ss, ts, player );
        harbor_viewer.set_selected_unit( unit.id() );
        co_await harbor_viewer.show();
        co_return false;
      }
    }
    // Need to forfeign movement points here to mark that we've
    // evolved this unit this turn (decreased its remaining
    // turns). If we didn't do this then this unit could keep
    // getting cycled through (e.g. `wait` cycles or after a
    // save-game reload) and would have its count decreased mul-
    // tiple times per turn.
    unit.forfeight_mv_points();
    co_return false;
  }

  if( unit.orders().holds<unit_orders::road>() ) {
    perform_road_work( ss, ts, unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders().holds<unit_orders::none>() );
      co_await ts.planes.get()
          .get_bottom<ILandViewPlane>()
          .ensure_visible_unit( id );
      co_await ts.gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( !unit.orders().holds<unit_orders::road>() );
  }

  if( unit.orders().holds<unit_orders::plow>() ) {
    PlowResult const plow_result =
        perform_plow_work( ss, ts, as_const( player ), unit );
    if( auto o =
            plow_result.get_if<PlowResult::cleared_forest>();
        o.has_value() && o->yield.has_value() ) {
      LumberYield const& yield = *o->yield;
      string const msg         = fmt::format(
          "Forest cleared near [{}].  [{}] lumber "
                  "added to colony's stockpile.",
          ss.colonies.colony_for( yield.colony_id ).name,
          yield.yield_to_add_to_colony );
      co_await ts.gui.message_box( msg );
    }
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders().holds<unit_orders::none>() );
      co_await ts.planes.get()
          .get_bottom<ILandViewPlane>()
          .ensure_visible_unit( id );
      co_await ts.gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( !unit.orders().holds<unit_orders::plow>() );
  }

  if( is_unit_in_port( ss.units, id ) ) {
    finish_turn( unit );
    co_return false; // do not ask for orders.
  }

  if( !is_unit_on_map_indirect( ss.units, id ) ) {
    finish_turn( unit );
    co_return false;
  }

  // Unit needs to ask for orders.
  co_return true;
}

// Accumulates results of advancing units on the high seas so
// that at the end of advancing all such units we can know
// whether e.g. to go to the harbor screen, which we only want to
// do once and not for each unit.
struct HighSeasStatus {
  bool arrived_in_harbor             = false;
  bool arrived_in_harbor_with_cargo  = false;
  UnitId last_unit_arrived_in_harbor = {};

  HighSeasStatus combined_with(
      HighSeasStatus const& rhs ) const {
    HighSeasStatus combined = *this;
    combined.arrived_in_harbor =
        combined.arrived_in_harbor || rhs.arrived_in_harbor;
    combined.arrived_in_harbor_with_cargo =
        combined.arrived_in_harbor_with_cargo ||
        rhs.arrived_in_harbor_with_cargo;
    if( rhs.last_unit_arrived_in_harbor != UnitId{} )
      combined.last_unit_arrived_in_harbor =
          rhs.last_unit_arrived_in_harbor;
    return combined;
  }
};

wait<HighSeasStatus> advance_high_seas_unit(
    SS& ss, TS& ts, Player& player, UnitId const unit_id ) {
  HighSeasStatus res;
  CHECK( is_unit_on_high_seas( ss, unit_id ) );
  Unit& unit = ss.units.unit_for( unit_id );
  CHECK( !should_remove_unit_from_queue( unit ) );
  e_high_seas_result const type =
      advance_unit_on_high_seas( ss, player, unit_id );
  switch( type ) {
    case e_high_seas_result::still_traveling:
      finish_turn( unit );
      break;
    case e_high_seas_result::arrived_in_new_world: {
      lg.debug( "unit {} has arrived in new world.", unit_id );
      maybe<Coord> const dst_coord =
          find_new_world_arrival_square(
              ss, ts, player,
              ss.units.harbor_view_state_of( unit_id )
                  .sailed_from );
      if( !dst_coord.has_value() ) {
        co_await ts.gui.message_box(
            "Unfortunately, while our [{}] has arrived in the "
            "new world, there are no appropriate water "
            "squares on which to place it.  We will try again "
            "next turn.",
            ss.units.unit_for( unit_id ).desc().name );
        finish_turn( unit );
        break;
      }
      ss.units.unit_for( unit_id ).clear_orders();
      maybe<UnitDeleted> const unit_deleted =
          co_await UnitOwnershipChanger( ss, unit_id )
              .change_to_map( ts, *dst_coord );
      // There are no LCR tiles on water squares.
      CHECK( !unit_deleted.has_value() );
      // This is not required, but it is for a good player ex-
      // perience. If there are more ships still in port then
      // select one of them, because ideally if there are ships
      // in port then when the player goes to the harbor view,
      // one of them should always be selected.
      update_harbor_selected_unit( ss.units, player );
      // Don't finish turn; will ask for orders.
      break;
    }
    case e_high_seas_result::arrived_in_harbor: {
      lg.debug( "unit {} has arrived in old world.", unit_id );
      finish_turn( unit );
      res.arrived_in_harbor           = true;
      res.last_unit_arrived_in_harbor = unit_id;
      res.arrived_in_harbor_with_cargo =
          ( unit.cargo()
                .count_items_of_type<Cargo::commodity>() > 0 );
      break;
    }
  }
  co_return res;
}

wait<> move_remaining_units( IEngine& engine, SS& ss, TS& ts,
                             Player& player,
                             PlayerTurnState::units& nat_units,
                             deque<UnitId>& q ) {
  while( !q.empty() ) {
    UnitId const id = q.front();

    // For e.g. units that are disbanded mid-loop.
    if( !ss.units.exists( id ) ) {
      q.pop_front();
      continue;
    }

    // High seas units are supposed to all be moved at the start
    // of the units turn in a separate location and should not be
    // given to us in the queue here, however we can still end up
    // with one if we are moving a ship in this function and tell
    // it to go to the high seas.
    if( is_unit_on_high_seas( ss, id ) ) {
      q.pop_front();
      continue;
    }

    // We need this check because units can be added into the
    // queue in this loop by user input.
    if( should_remove_unit_from_queue(
            ss.units.unit_for( id ) ) ) {
      q.pop_front();
      continue;
    }

    bool should_ask =
        co_await advance_unit( engine, ss, ts, player, id );
    if( !should_ask ) {
      q.pop_front();
      continue;
    }

    // We have a unit that needs to ask the user for orders. This
    // will open things up to player input not only to the unit
    // that needs orders, but to all units on the map, and will
    // update the queue with any changes that result. This func-
    // tion will return on any player input (e.g. clearing the
    // orders of another unit) and so we might need to circle
    // back to this line a few times in this while loop until we
    // get the order for the unit in question (unless the player
    // activates another unit).
    co_await query_unit_input( engine, id, ss, ts, player,
                               nat_units );
    // !! The unit may no longer exist at this point, e.g. if
    // they were disbanded or if they lost a battle to the na-
    // tives.
    //
    // NOTE: we should not pop the unit off of the queue here be-
    // cause the above function may have inserted additional
    // units into the front of the queue, so the front unit may
    // not be as it was at the start of this loop.
  }
}

// The idea of this function is that we advance all of the high
// seas units first, then if any have made it to the harbor then
// we take the player to the harbor. That way we don't see it re-
// peatedly for every unit.
wait<> move_high_seas_units( IEngine& engine, SS& ss, TS& ts,
                             Player& player, deque<UnitId>& q ) {
  HighSeasStatus status_union;
  while( !q.empty() ) {
    UnitId const id = q.front();
    // I think this should always hold here...
    CHECK( ss.units.exists( id ) );

    if( !is_unit_on_high_seas( ss, id ) )
      // We've exausted the high seas units, since the caller
      // should have put them all at the front of the queue.
      break;

    CHECK( !should_remove_unit_from_queue(
        ss.units.unit_for( id ) ) );

    HighSeasStatus const status =
        co_await advance_high_seas_unit( ss, ts, player, id );
    status_union = status_union.combined_with( status );

    // Should not have inserted any new units.
    CHECK( q.front() == id );

    // NOTE: it is possible in the future that the unit at the
    // front of the queue might have been destroyed, depending on
    // how we handle the seizing of ships by the crown after in-
    // dependence.

    q.pop_front();
  }

  if( status_union.arrived_in_harbor ) {
    CHECK( status_union.last_unit_arrived_in_harbor !=
           UnitId{} );
    if( status_union.arrived_in_harbor_with_cargo )
      co_await show_woodcut_if_needed(
          player, ts.euro_minds()[player.type],
          e_woodcut::cargo_from_the_new_world );
    HarborViewer harbor_viewer( engine, ss, ts, player );
    harbor_viewer.set_selected_unit(
        status_union.last_unit_arrived_in_harbor );
    co_await harbor_viewer.show();
  }
}

wait<> units_turn_one_pass( IEngine& engine, SS& ss, TS& ts,
                            Player& player,
                            PlayerTurnState::units& nat_units,
                            deque<UnitId>& q ) {
  // There may be some units in the queue that e.g. we disbanded
  // while in view mode just before having called this method.
  remove_deleted_units( ss, q );
  // Put all the high-seas units at the start of the q, pre-
  // serving order. Return the iterator to the first non high
  // seas unit, which we then use to determine how many high seas
  // units there are.
  auto const first_non_high_seas_iter = stable_partition(
      q.begin(), q.end(),
      bind_front( is_unit_on_high_seas, ref( ss.as_const ) ) );
  if( first_non_high_seas_iter != q.begin() ) {
    co_await move_high_seas_units( engine, ss, ts, player, q );
    // Empty the queue and return so that the entire unit queue
    // gets remade, that way any ships that have just made it to
    // the new world from the high seas will ask for orders in
    // the correct order. Note that when this function is called
    // next, there should be no high seas units in it since
    // they've all just been moved, thus we should not get into
    // this branch.
    q = {};
    co_return;
  }
  co_await move_remaining_units( engine, ss, ts, player,
                                 nat_units, q );
}

wait<> units_turn( IEngine& engine, SS& ss, TS& ts,
                   Player& player,
                   PlayerTurnState::units& nat_units ) {
  auto& st = nat_units;
  auto& q  = st.q;

  // NOTE: this function needs to support the case where a game
  // is loaded already in view mode, thus we must be ready to
  // enter into it before asking units for orders if needed.
  auto input_or_view_mode = [&]() -> wait<> {
    ViewModeOptions view_mode_options;

    auto const view_mode = [&]() -> wait<> {
      co_await show_view_mode( engine, ss, ts, player, nat_units,
                               view_mode_options );
    };

    if( nat_units.view_mode ) co_await view_mode();
    // This should have been reset upon exiting from view mode.
    CHECK( !nat_units.view_mode );

    while( true ) {
      try {
        co_await units_turn_one_pass( engine, ss, ts, player,
                                      nat_units, q );
        co_return;
      } catch( view_mode_interrupt const& e ) {
        view_mode_options = e.options;
      };
      co_await view_mode();
    }
  };

  // Here we will keep reloading all of the units (that still
  // need to move) and making passes over them in order make sure
  // that we systematically get to all units that need to move
  // this turn, including the ones that were activated during the
  // turn. For example, during the course of moving units, the
  // user might activate a unit in the land view, or they might
  // open a colony view and active units there, or a new unit
  // might be created, etc. There are many ways that this can
  // happen, and this will ensure that we get to all of those
  // units in a systematic way without having to handle each sit-
  // uation specially.
  //
  // That said, we also need this to work right when there are
  // already some units in the queue on the first iteration, as
  // would be the case just after deserialization.
  while( true ) {
    co_await input_or_view_mode();
    CHECK( q.empty() );
    // Refill the queue.
    vector<UnitId> units =
        euro_units_all( ss.units, player.type );
    util::sort_by_key( units, []( auto id ) { return id; } );
    erase_if( units, [&]( UnitId id ) {
      return should_remove_unit_from_queue(
          ss.units.unit_for( id ) );
    } );
    if( units.empty() ) break;
    q = deque( units.begin(), units.end() );
  }
}

/****************************************************************
** Per-Colony Turn Processor
*****************************************************************/
wait<> colonies_turn( IEngine& engine, SS& ss, TS& ts,
                      Player& player ) {
  RealColonyEvolver const colony_evolver( ss, ts );
  RealColonyNotificationGenerator const
      colony_notification_generator;
  HarborViewer harbor_viewer( engine, ss, ts, player );
  co_await evolve_colonies_for_player(
      ss, ts, player, colony_evolver, harbor_viewer,
      colony_notification_generator );
}

// Here we do things that must be done once per turn but where we
// want the colonies to be evolved first.
wait<> post_colonies( SS& ss, TS& ts, Player& player ) {
  // Founding fathers.
  if( player.revolution.status <
      e_revolution_status::declared ) {
    co_await pick_founding_father_if_needed( ss, ts, player );
    maybe<e_founding_father> const new_father =
        check_founding_fathers( ss, player );
    if( new_father.has_value() ) {
      co_await play_new_father_cut_scene( ts, player,
                                          *new_father );
      // This will affect any one-time changes that the new
      // father causes. E.g. for John Paul Jones it will create
      // the frigate.
      on_father_received( ss, ts, player, *new_father );
    }
  }

  // Evolve rebel sentiment. This must be done after colonies are
  // evolved so that the rebel sentiment level during the turn
  // will be consistent with the SoL of the various colonies. It
  // must not be done, though, after independence has been de-
  // clared since in the OG the rebel sentiment gets frozen at
  // that point and later contributes to the player's score.
  if( player.revolution.status <
      e_revolution_status::declared ) {
    auto const report = update_rebel_sentiment(
        player, updated_rebel_sentiment( ss.as_const,
                                         as_const( player ) ) );
    if( should_show_rebel_sentiment_report(
            ss.as_const, as_const( player ), report.nova ) )
      co_await show_rebel_sentiment_change_report(
          player, ts.euro_minds()[player.type], report );
  }

  // Check if we need to do the war of succession. This must be
  // done immediately after evolving rebel sentiment, since the
  // game should not be allowed to be in a state where rebel sen-
  // timent is >= 50% and the war of succession has not happened.
  // Actually in the NG it wouldn't really matter if this were
  // violated, but it is just to keep consistent with the OG, for
  // which it was important.
  if( should_do_war_of_succession( as_const( ss ),
                                   as_const( player ) ) ) {
    WarOfSuccessionNations const nations =
        select_players_for_war_of_succession( ss.as_const );
    WarOfSuccessionPlan const plan =
        war_of_succession_plan( ss.as_const, nations );
    do_war_of_succession( ss, ts, player, plan );
    co_await do_war_of_succession_ui_seq( ts, plan );
  }

  // Try to determine which turn we're on relative to the one
  // where we declared independence. This would be made easier if
  // we were to just record the turn where independence was de-
  // clared, but the OG does not record that, so would lose in-
  // terconvtibility of sav files. So we have these two flags
  // that the OG does have, and we piece together where we are
  // from that.
  switch( post_declaration_turn( player ) ) {
    using enum e_turn_after_declaration;
    case zero:
      // Do nothing here.
      break;
    case first: {
      // Mobilize continental army.
      player.revolution.continental_army_mobilized = true;
      vector<ColonyId> const colonies =
          ss.colonies.for_player( player.type );
      for( ColonyId const colony_id : colonies ) {
        Colony const& colony =
            ss.colonies.colony_for( colony_id );
        ContinentalPromotion const promotion =
            compute_continental_promotion( ss.as_const, player,
                                           colony_id );
        do_continental_promotion( ss, ts, promotion );
        co_await ts.planes.get()
            .get_bottom<ILandViewPlane>()
            .ensure_visible( colony.location );
        co_await continental_promotion_ui_seq(
            ss.as_const, ts.gui, promotion, colony_id );
      }
      break;
    }
    case second:
      player.revolution.gave_independence_war_hints = true;
      break;
    case done:
      break;
  }
}

/****************************************************************
** Per-Nation Turn Processor
*****************************************************************/
// Here we do things that must be done once at the start of each
// nation's turn but where the player can't save the game until
// they are complete.
wait<> player_start_of_turn( SS& ss, TS& ts, Player& player ) {
  recompute_fog_for_all_players( ss, ts );

  // Unsentry any units that are directly on the map and which
  // are sentry'd but have foreign units in an adjacent square.
  // Normally this isn't necessary, since if a unit is sentried
  // and a foreign unit moves adjacent to it then the former unit
  // will be unsentried. However, there are cases where, e.g. a
  // unit moves next to a fortified unit and the former unit sen-
  // tries. That unit needs to be woken up at the start of the
  // next turn. We could just prevent sentrying next to a foreign
  // unit, but 1) that's not what the OG does, and that might
  // have other side effects, e.g. you could not sentry a unit in
  // a colony to board a ship if there were foreign units nearby.
  unsentry_units_next_to_foreign_units( ss, player.type );

  // Evolve market prices.
  if( ss.turn.time_point.turns >
      config_turn.turns_to_wait.market_evolution ) {
    // This will actually change the prices, then will return
    // info about which ones it changed.
    refl::enum_map<e_commodity, PriceChange> changes =
        evolve_player_prices( ss, player );
    for( e_commodity comm : refl::enum_values<e_commodity> )
      if( changes[comm].delta != 0 )
        co_await display_price_change_notification(
            ts, player, changes[comm] );
  }

  // Check for tax events (typically increases).
  co_await start_of_turn_tax_check( ss, ts, player );

  // TODO:
  //
  //   * Sending units. NOTE: when your home country does this
  //     there is a probability for an immediate large tax in-
  //     crease; see config/tax file.
  //
}

// This is for things that should be done after a nation's turn
// ends. This means even after the "end of turn" is clicked, if
// that happens to flash on a given turn.
wait<> post_player( SS& ss, TS& ts, Player& player ) {
  // Evolve royal money and check if we need to add a new REF
  // unit.
  RoyalMoneyChange const change = evolved_royal_money(
      ss.settings.game_setup_options.difficulty,
      as_const( player.royal_money ) );
  apply_royal_money_change( player, change );
  if( change.new_unit_produced ) {
    e_expeditionary_force_type const type = select_next_ref_type(
        player.revolution.expeditionary_force );
    add_ref_unit( player.revolution.expeditionary_force, type );
    co_await add_ref_unit_ui_seq( ts.euro_minds()[player.type],
                                  type );
  }
}

// Processes the current state and returns the next state.
wait<PlayerTurnState> player_turn_iter(
    IEngine& engine, SS& ss, TS& ts, e_player const player_type,
    PlayerTurnState& st ) {
  Player& player =
      player_for_player_or_die( ss.players, player_type );

  SWITCH( st ) {
    CASE( not_started ) {
      base::print_bar( '-',
                       fmt::format( "[ {} ]", player_type ) );
      co_await player_start_of_turn( ss, ts, player );
      co_return PlayerTurnState::colonies{};
    }
    CASE( colonies ) {
      co_await colonies_turn( engine, ss, ts, player );
      co_await post_colonies( ss, ts, player );
      co_return PlayerTurnState::units{};
    }
    CASE( units ) {
      co_await units_turn( engine, ss, ts, player, units );
      CHECK( units.q.empty() );
      if( !units.skip_eot ) co_return PlayerTurnState::eot{};
      if( ss.settings.in_game_options.game_menu_options
              [e_game_menu_option::end_of_turn] )
        // As in the OG, this setting means "always stop on end
        // of turn," even we otherwise wouldn't have.
        co_return PlayerTurnState::eot{};
      co_return PlayerTurnState::post{};
    }
    CASE( eot ) {
      SWITCH( co_await end_of_turn( engine, ss, ts, player ) ) {
        CASE( not_done_yet ) {
          co_return PlayerTurnState::eot{};
        }
        CASE( proceed ) { co_return PlayerTurnState::post{}; }
        CASE( return_to_units ) {
          PlayerTurnState::units units;
          if( return_to_units.first_to_ask.has_value() )
            units.q.push_back( *return_to_units.first_to_ask );
          co_return units;
        }
      }
      SHOULD_NOT_BE_HERE; // for gcc.
    }
    CASE( post ) {
      co_await post_player( ss, ts, player );
      co_return PlayerTurnState::finished{};
    }
    CASE( finished ) { SHOULD_NOT_BE_HERE; }
  }
}

wait<> nation_turn( IEngine& engine, SS& ss, TS& ts,
                    e_player player, PlayerTurnState& st ) {
  CHECK( ss.players.players[player].has_value(),
         "nation {} does not exist.", player );
  switch( ss.players.players[player]->control ) {
    case e_player_control::withdrawn:
      st = PlayerTurnState::finished{};
      break;
    case e_player_control::human:
      while( !st.holds<PlayerTurnState::finished>() )
        st = co_await player_turn_iter( engine, ss, ts, player,
                                        st );
      break;
    case e_player_control::ai:
      // TODO: Until we have AI.
      st = PlayerTurnState::finished{};
      break;
  }
}

/****************************************************************
** Intervention Force.
*****************************************************************/
wait<> do_intervention_force_turn(
    IEngine&, SS& ss, TS& ts, e_player const player_type,
    TurnCycle::intervention const& ) {
  UNWRAP_CHECK( player, ss.players.players[player_type] );
  if( !player.revolution.intervention_force_deployed ) {
    if( should_trigger_intervention( ss.as_const,
                                     as_const( player ) ) ) {
      trigger_intervention( player );
      auto const intervention_player =
          select_nation_for_intervention( player.nation );
      co_await intervention_forces_triggered_ui_seq(
          ss, ts.gui, player.nation, intervention_player );
    }
    // !! No return here since we want to deploy on the same turn
    // as the OG does.
  }

  if( player.revolution.intervention_force_deployed ) {
    auto const forces = pick_forces_to_deploy( player );
    lg.debug( "chose intervention forces: {}", forces );
    if( forces.has_value() ) {
      auto const target = find_intervention_deploy_tile(
          ss, ts.rand, ts.connectivity, player );
      if( target.has_value() ) {
        UnitId const ship_id = deploy_intervention_forces(
            ss, ts, *target, *forces );
        Colony const& colony =
            ss.colonies.colony_for( target->colony_id );
        e_nation const intervention_nation =
            select_nation_for_intervention( player.nation );
        co_await intervention_forces_deployed_ui_seq(
            ts, colony, intervention_nation );
        co_await animate_move_intervention_units_into_colony(
            ss, ts, ship_id, colony );
        move_intervention_units_into_colony( ss, ts, ship_id,
                                             colony );
      }
    }
  }
}

/****************************************************************
** Turn Processor
*****************************************************************/
// Here we do things that must be done once at the start of each
// full turn cycle but where the player can't save the game until
// they are complete.
void start_of_turn_cycle( SS& ss ) {
  // This will evolve the internal market model state of the
  // processed goods (rum, cigars, cloth, coats). It is done once
  // per turn cycle instead of once per player turn because said
  // model state is shared among all players. That said, the
  // price movement of these goods will happen at the start of
  // each player turn for that respective player.
  if( ss.turn.time_point.turns >
      config_turn.turns_to_wait.market_evolution )
    evolve_group_model_volumes( ss );

  // Note: resetting of the european units' movement points must
  // be done in this function so that it does not get redone if a
  // game is loaded that was mid-turn. For the natives, that can
  // be done in the "natives" section because we know that the
  // game will never be saved during the natives' turn.
  map_all_euro_units( ss.units, [&]( Unit& unit ) {
    UNWRAP_CHECK( player,
                  ss.players.players[unit.player_type()] );
    unit.new_turn( player );
  } );
}

// Processes teh current state and returns the next state.
wait<TurnCycle> next_turn_iter( IEngine& engine, SS& ss,
                                TS& ts ) {
  TurnState& turn  = ss.turn;
  TurnCycle& cycle = ss.turn.cycle;
  // The "visibility" here determines from whose point of view
  // the map is drawn with respect to which tiles are hidden.
  // This will potentially redraw the map (if necessary) to align
  // with the player from whose perspective we are currently
  // viewing the map (if any) as specified in the land view
  // state. This is a function of the player whose turn it cur-
  // rently is (or the human status of the players if it is no
  // european nations' turn) and so we need to update it each
  // time the turn cycle changes.
  update_map_visibility(
      ts, player_for_role( ss, e_player_role::viewer ) );
  SWITCH( cycle ) {
    CASE( not_started ) {
      start_of_turn_cycle( ss );
      co_return TurnCycle::natives{};
    }
    CASE( natives ) {
      recompute_fog_for_all_players( ss, ts );
      co_await natives_turn( ss, ts, RealRaid( ss, ts ),
                             RealTribeEvolve( ss, ts ) );
      if( auto const player = find_first_player_to_move( ss );
          player.has_value() )
        co_return TurnCycle::player{ .type = *player };
      co_return TurnCycle::end_cycle{};
    }
    CASE( player ) {
      co_await nation_turn( engine, ss, ts, player.type,
                            player.st );
      if( auto const next =
              find_next_player_to_move( ss, player.type );
          next.has_value() )
        co_return TurnCycle::player{ .type = *next };
      co_return TurnCycle::intervention{};
    }
    CASE( intervention ) {
      auto const player = human_player_that_declared( ss );
      if( player.has_value() )
        co_await do_intervention_force_turn(
            engine, ss, ts, *player, intervention );
      co_return TurnCycle::end_cycle{};
    }
    CASE( end_cycle ) {
      co_await advance_time( ts.gui, turn.time_point );
      co_return TurnCycle::finished{};
    }
    CASE( finished ) { SHOULD_NOT_BE_HERE; }
  }
}

// Runs through the various phases of a single turn.
wait<> next_turn( IEngine& engine, SS& ss, TS& ts ) {
  TurnCycle& cycle = ss.turn.cycle;
  ts.planes.get().get_bottom<ILandViewPlane>().start_new_turn();
  base::print_bar( '=', "[ Starting Turn ]" );
  while( !cycle.holds<TurnCycle::finished>() )
    cycle = co_await next_turn_iter( engine, ss, ts );
  // The default-constructed cycle represents a new turn where
  // nothing yet has been done. Do this at the end of the cycle
  // so that we don't destroy the turn state after having loaded
  // a saved game.
  cycle = {};
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
// Runs through multiple turns.
wait<> turn_loop( IEngine& engine, SS& ss, TS& ts ) {
  while( true ) {
    try {
      co_await next_turn( engine, ss, ts );
    } catch( top_of_turn_loop const& ) {}
  }
}

} // namespace rn
