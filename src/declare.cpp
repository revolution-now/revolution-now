/****************************************************************
**declare.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-28.
*
* Description: Declaration of independence action.
*
*****************************************************************/
#include "declare.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "rebel-sentiment.hpp"
#include "revolution-status.hpp"
#include "ts.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/revolution.rds.hpp"

// ss
#include "revolution.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

bool rebellion_large_enough_to_declare(
    SettingsState const& settings, Player const& player ) {
  return player.revolution.rebel_sentiment >=
         config_revolution.declaration
             .human_required_rebel_sentiment_percent
                 [settings.game_setup_options.difficulty]
             .percent;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
maybe<e_player> human_player_that_declared( SSConst const& ss ) {
  using enum e_revolution_status;
  // The players state validation should have ensured that there
  // is at most one human player that has declared.
  for( auto const& [type, player] : ss.players.players )
    if( player.has_value() && player->human &&
        player->revolution.status >= declared )
      return type;
  return nothing;
}

valid_or<e_declare_rejection> can_declare_independence(
    SSConst const& ss, Player const& player ) {
  switch( player.revolution.status ) {
    using enum e_revolution_status;
    using enum e_declare_rejection;
    case not_declared: {
      for( auto const& [type, other] : ss.players.players )
        if( other.has_value() && type != player.type )
          if( other->revolution.status >= declared )
            return other_human_already_declared;
      if( rebellion_large_enough_to_declare( ss.settings,
                                             player ) )
        return valid;
      return rebel_sentiment_too_low;
    }
    case declared:
      return already_declared;
    case won:
      return already_won;
  }
}

wait<> show_declare_rejection_msg(
    IGui& gui, e_declare_rejection const reason ) {
  switch( reason ) {
    using enum e_declare_rejection;
    case rebel_sentiment_too_low:
      co_await gui.message_box( "Rebel sentiment too low." );
      break;
    case already_declared:
      co_await gui.message_box(
          "We are already fighting the war of independence." );
      break;
    case other_human_already_declared:
      co_await gui.message_box(
          "We cannot declare independence as another non-AI "
          "player has already declared." );
      break;
    case already_won:
      co_await gui.message_box(
          "We have already won our independence." );
      break;
  }
}

wait<ui::e_confirm> ask_declare( IGui& gui,
                                 Player const& player ) {
  YesNoConfig const config{
    .msg = format(
        "Shall we declare independence from [{}], Your "
        "Excellency?  Doing so will end this turn and "
        "immediately put us at war with the crown.",
        config_nation.players[player.type].country_name ),
    .yes_label =
        "Yes! Give me liberty or give me death! ([Leave])",
    .no_label =
        "Never! Eternal Glory to His Majesty the King! "
        "([Remain])",
    .no_comes_first = true };
  maybe<ui::e_confirm> const answer =
      co_await gui.optional_yes_no( config );
  co_return answer.value_or( ui::e_confirm::no );
}

wait<> declare_independence_ui_sequence_pre( SSConst const&,
                                             TS& ts,
                                             Player const& ) {
  co_await ts.gui.message_box(
      "(signing of signature on declaration)" );
}

DeclarationResult declare_independence( SS& ss, TS&,
                                        Player& player ) {
  DeclarationResult res;
  e_difficulty const difficulty =
      ss.as_const.settings.game_setup_options.difficulty;

  // Step: Set declaration status.
  player.revolution.status = e_revolution_status::declared;

  // Step: Add backup force.
  player.revolution.intervention_force =
      config_revolution.intervention_forces
          .unit_counts[difficulty];

  // TODO:
  //
  // * Make sure that there is at least one Man-o-War if there
  //   are any ref units to bring. The OG appears to increase
  //   this from zero to one if it is zero.
  // * Eliminate all foreign units.
  // * Mark foreign players as "withdrawn"; they no longer get
  //   evolved, probably at the colony level either. The players
  //   can't be deleted because their colonies persist.
  // * Seize all ships on the high seas and in europe and ensure
  //   that the europe screen changes.
  // * Ensure that cross accumulation stops as well as immigra-
  //   tion.
  // * Reset bell count to zero.
  // * Promote continental armies in colonies. This actually ap-
  //   pears to happen at the start of the new turn.
  // * Reset founding fathers state.
  // * End the turn of the player. Ensure that they get an
  //   end-of-turn if needed.
  // * On the next turn, show the intervention forces intro along
  //   with the other intro messages about the REF combat strate-
  //   gies.

  return res;
}

wait<> declare_independence_ui_sequence_post(
    SSConst const&, TS& ts, Player const&,
    DeclarationResult const& ) {
  co_await ts.gui.message_box(
      "Continental Congress signs [Declaration of "
      "Independence]! (more description of things that "
      "happen)" );
}

} // namespace rn
