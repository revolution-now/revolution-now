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
#include "interrupts.hpp"
#include "map-view.hpp"
#include "ts.hpp"

// config
#include "config/turn.rds.hpp"

// ss
#include "ss/events.rds.hpp"
#include "ss/nation.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"

using namespace std;

namespace rn {

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
    // Don't resurrect the nation that withdrew in the War of
    // Succession, if there was one.
    if( nation_for( player_type ) ==
        ss.events.war_of_succession_done )
      continue;
    if( player->control == e_player_control::inactive )
      player->control = e_player_control::ai;
  }
}

void do_keep_playing_after_timeout( SS&, TS& ) {
  // Just keep things the way they are.
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

wait<> check_time_up( SS& ss, TS& ts ) {
  auto const& time_limit =
      config_turn.game_ending.deadline_for_winning;
  if( !time_limit.has_value() ) co_return;
  // We want "not equal" here so that we stop checking this dead-
  // line after the year/season of the deadline, in case the
  // player decides to continue playing.
  if( ss.turn.time_point.year != *time_limit ||
      ss.turn.time_point.season != e_season::autumn )
    co_return;
  auto const declared =
      human_player_that_declared( ss.as_const );
  if( declared.has_value() )
    // TODO: add more here.
    co_await ts.gui.message_box(
        "{} {} resigns after {} years of dedicated service to "
        "Crown." );
  else
    // TODO: add more here.
    co_await ts.gui.message_box(
        "{} {} surrenders to the Crown." );
  // TODO: record loss.
  // TODO: do scoring.
  e_keep_playing const keep_playing =
      co_await ask_keep_playing( ts.gui );
  switch( keep_playing ) {
    case e_keep_playing::no:
      throw main_menu_interrupt{};
    case e_keep_playing::yes:
      do_keep_playing_after_timeout( ss, ts );
      break;
  }
}

} // namespace rn
