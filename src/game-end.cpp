/****************************************************************
**game-end.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Handles general things related to ending the game.
*
*****************************************************************/
#include "game-end.hpp"

// Revolution Now
#include "declare.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "interrupts.hpp"
#include "map-view.hpp"
#include "ref.hpp"
#include "roles.hpp"
#include "string.hpp"
#include "ts.hpp"

// config
#include "config/turn.rds.hpp"

// ss
#include "ss/events.rds.hpp"
#include "ss/nation.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/revolution.rds.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.rds.hpp"

// base
#include "base/logger.hpp"
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

using ::refl::enum_value_name;
using ::refl::enum_values;

// The player can also opt to continue playing after having run
// out of time, but this should not be called in that case. We
// only want to reward the player by revealing the entire map if
// they've won.
void do_keep_playing_after_winning( SS& ss, TS& ts ) {
  reveal_entire_map( ss, ts );
  // Bring back an AI players that were disabled upon declara-
  // tion. We may end up allowing the player to control that fea-
  // ture and not withdraw the foreign nations upon declaration
  // (since the OG appears to have only done that due to tech-
  // nical constraints on the number of allowed units). But ei-
  // ther way, this should not cause a problem.
  for( auto& [player_type, player] : ss.players.players ) {
    if( !player.has_value() ) continue;
    if( is_ref( player_type ) ) continue;
    // Don't resurrect the nation that withdrew in the War of
    // Succession, if there was one.
    if( nation_for( player_type ) ==
        ss.events.war_of_succession_done )
      continue;
    if( player->control == e_player_control::human ) continue;
    if( player->control == e_player_control::inactive )
      // This will convert any previous human players to AI,
      // since we don't know whether they started off as human or
      // not. This would only matter in a non-standard game mode,
      // and if the player really cares then they can put that
      // player back to human via the cheat menu. In a normal
      // game this does what we want.
      player->control = e_player_control::ai;
  }
}

wait<e_keep_playing> ask_keep_playing( IGui& gui ) {
  YesNoConfig const config{ .msg       = "Keep Playing?",
                            .yes_label = "Yes",
                            .no_label  = "No" };
  maybe<ui::e_confirm> const answer =
      co_await gui.optional_yes_no( config );
  if( answer != ui::e_confirm::yes )
    co_return e_keep_playing::no;
  co_return e_keep_playing::yes;
}

} // namespace

bool check_time_up( SSConst const& ss ) {
  auto const& time_limit =
      ss.settings.game_setup_options.customized_rules
          .deadline_for_winning;
  if( !time_limit.has_value() ) return false;

  // If there are no human players then this doesn't really ap-
  // ply. A different method will be chosen to end the game
  // there.
  auto const primary_human =
      player_for_role( ss, e_player_role::primary_human );
  if( !primary_human.has_value() ) return false;

  // We want "not equal" here so that we stop checking this dead-
  // line after the year/season of the deadline, in case the
  // player decides to continue playing.
  if( ss.turn.time_point.year != *time_limit ||
      ss.turn.time_point.season != e_season::autumn )
    return false;

  // We might have just arrived at the time limit, but maybe the
  // human player has already won and opted to keep playing. Need
  // to check this before we check declaration status below.
  for( e_player const player : enum_values<e_player> )
    if( ss.players.players[player].has_value() )
      if( ss.players.players[player]->control ==
          e_player_control::human )
        if( ss.players.players[player]->revolution.status ==
            e_revolution_status::won )
          return false;

  return true;
}

wait<e_game_end> do_time_up( SS& ss, IGui& gui ) {
  auto const primary_human =
      player_for_role( ss, e_player_role::primary_human );
  if( !primary_human.has_value() )
    co_return e_game_end::not_ended;

  string const title =
      base::capitalize_initials( enum_value_name(
          ss.settings.game_setup_options.difficulty ) );

  if( auto const declared =
          human_player_that_declared( ss.as_const );
      declared.has_value() ) {
    UNWRAP_CHECK_T( Player const& player,
                    ss.players.players[*declared] );
    // TODO: add more here.
    co_await gui.message_box( "{} {} surrenders to the Crown.",
                              title, player.name );
  } else {
    // Should have been checked above.
    CHECK( primary_human.has_value() );
    UNWRAP_CHECK_T( Player const& player,
                    ss.players.players[*primary_human] );
    int const years_of_service =
        std::max( ss.turn.time_point.year -
                      config_turn.game_start.starting_year,
                  0 );
    // TODO: add more here.
    co_await gui.message_box(
        "{} {} resigns after {} years of dedicated service to "
        "the Crown.",
        title, player.name, years_of_service );
  }

  // All human players lose.
  for( e_player const player : enum_values<e_player> )
    if( ss.players.players[player].has_value() )
      if( ss.players.players[player]->control ==
          e_player_control::human )
        ss.players.players[player]->revolution.status =
            e_revolution_status::lost;

  // TODO: record loss.
  // TODO: do scoring.
  e_keep_playing const keep_playing =
      co_await ask_keep_playing( gui );
  switch( keep_playing ) {
    case e_keep_playing::no:
      co_return e_game_end::ended_and_back_to_main_menu;
    case e_keep_playing::yes:
      co_return e_game_end::ended_and_player_continues;
  }
}

wait<e_game_end> check_for_ref_win( SS& ss, TS& ts,
                                    Player const& ref_player ) {
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_for(
                      nation_for( ref_player.type ) )] );
  CHECK( colonial_player.revolution.status >=
         e_revolution_status::declared );
  if( colonial_player.revolution.status !=
      e_revolution_status::declared )
    co_return e_game_end::not_ended;
  auto const won = ref_should_win(
      ss, ts.map_updater().connectivity(), ref_player );
  if( !won.has_value() ) co_return e_game_end::not_ended;
  do_ref_win( ss, ref_player );
  co_await ref_win_ui_routine( ss, ts.gui, ref_player, *won );
  co_return e_game_end::ended_and_back_to_main_menu;
}

wait<e_game_end> check_for_ref_forfeight( SS& ss, TS& ts,
                                          Player& ref_player ) {
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_for(
                      nation_for( ref_player.type ) )] );
  CHECK( colonial_player.revolution.status >=
         e_revolution_status::declared );
  if( colonial_player.revolution.status !=
      e_revolution_status::declared )
    co_return e_game_end::not_ended;
  if( !ref_should_forfeight( ss.as_const, ref_player ) )
    co_return e_game_end::not_ended;
  lg.info( "the REF is forfeighting." );
  do_ref_forfeight( ss, ref_player );
  co_await ref_forfeight_ui_routine( ss.as_const, ts.gui,
                                     ref_player );
  e_keep_playing const keep_playing =
      co_await ask_keep_playing( ts.gui );
  switch( keep_playing ) {
    case e_keep_playing::no:
      co_return e_game_end::ended_and_back_to_main_menu;
    case e_keep_playing::yes:
      do_keep_playing_after_winning( ss, ts );
      co_return e_game_end::ended_and_player_continues;
  }
}

} // namespace rn
