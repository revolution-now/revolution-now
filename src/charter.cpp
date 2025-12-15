/****************************************************************
**charter.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-14.
*
* Description: Handles the "end of charter" mechanic.
*
*****************************************************************/
#include "charter.hpp"

// Revolution Now
#include "igui.hpp"

// config
#include "config/turn.rds.hpp"

// ss
#include "revolution.rds.hpp"
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.rds.hpp"

namespace rn {

namespace {

using namespace std;

int human_player_count( SSConst const& ss ) {
  int total = 0;
  for( auto const& [type, player] : ss.players.players ) {
    if( !player.has_value() ) continue;
    if( player->control == e_player_control::human ) ++total;
  }
  return total;
}

[[nodiscard]] bool eligible_for_charter_end(
    SSConst const& ss, e_player const player_type ) {
  if( !config_turn.charter.enable_charter_term_limit )
    return false;
  if( human_player_count( ss ) != 1 ) return false;
  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[player_type] );
  if( player.control != e_player_control::human ) return false;
  if( player.revolution.status >= e_revolution_status::declared )
    // The charter never gets revoked after declaration; instead
    // if the player loses all of their colonies then it is in-
    // terpreted as an REF win.
    return false;
  if( is_ref( player_type ) ) return false;
  return true;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
int charter_end_year() {
  return config_turn.charter.charter_end_without_colonies;
}

bool should_warn_about_charter_end(
    SSConst const& ss, e_player const player_type ) {
  if( !eligible_for_charter_end( ss, player_type ) )
    return false;
  int const year = ss.turn.time_point.year;
  int const end  = charter_end_year();
  int const delta =
      config_turn.charter.warn_charter_end_without_colonies;
  int const warn_year = end - delta;
  return year >= warn_year;
}

bool should_end_charter( SSConst const& ss,
                         e_player const player_type ) {
  if( !eligible_for_charter_end( ss, player_type ) )
    return false;
  int const year = ss.turn.time_point.year;
  int const end  = charter_end_year();

  bool const at_or_past_end = year >= end;
  bool const has_no_colonies =
      ss.colonies.for_player( player_type ).empty();

  return at_or_past_end && has_no_colonies;
}

wait<> end_charter_ui_seq( SSConst const& ss, IGui& gui,
                           e_player const player_type ) {
  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[player_type] );
  string const msg = format(
      "{} {}. Our time in the New World has achieved little of "
      "consequence. We will be promptly ending your tenure as "
      "Viceroy in favour of someone more capable. It is, "
      "though, still your privilege to kiss our royal pinky "
      "ring.",
      IGui::identifier_to_display_name( base::to_str(
          ss.settings.game_setup_options.difficulty ) ),
      player.name );
  co_await gui.message_box( "{}", msg );
}

} // namespace rn
