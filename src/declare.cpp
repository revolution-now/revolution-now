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
#include "agents.hpp"
#include "co-wait.hpp"
#include "ieuro-agent.hpp"
#include "igui.hpp"
#include "player-mgr.hpp"
#include "rebel-sentiment.hpp"
#include "revolution-status.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/revolution.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "revolution.rds.hpp"
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/conv.hpp"

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
    if( player.has_value() &&
        player->control == e_player_control::human &&
        player->revolution.status >= declared )
      return type;
  return nothing;
}

valid_or<e_declare_rejection> can_declare_independence(
    SSConst const& ss, Player const& player ) {
  using enum e_declare_rejection;
  if( is_ref( player.type ) ) return ref_cannot_declare;
  switch( player.revolution.status ) {
    using enum e_revolution_status;
    case not_declared: {
      for( auto const& [type, other] : ss.players.players )
        if( other.has_value() && type != player.type )
          if( other->revolution.status >= declared )
            return other_human_already_declared;
      if( !rebellion_large_enough_to_declare( ss.settings,
                                              player ) )
        return rebel_sentiment_too_low;
      return valid;
    }
    case declared:
      return already_declared;
    case won:
      return already_won;
  }
}

wait<> show_declare_rejection_msg(
    SSConst const& ss, Player const& player, IGui& gui,
    e_declare_rejection const reason ) {
  switch( reason ) {
    using enum e_declare_rejection;
    case rebel_sentiment_too_low:
      co_await gui.message_box(
          "We cannot declare independence because [Rebel "
          "Sentiment] in {} is too low. We must have at least "
          "[{}%] confidence in the revolution in order to "
          "declare it.",
          player.new_world_name.value_or( "the New World" ),
          config_revolution.declaration
              .human_required_rebel_sentiment_percent
                  [ss.settings.game_setup_options.difficulty]
              .percent );
      break;
    case already_declared:
      co_await gui.message_box(
          "We are already fighting the War of Independence." );
      break;
    case other_human_already_declared:
      co_await gui.message_box(
          "We cannot declare independence as another "
          "human-controlled player has already declared." );
      break;
    case already_won:
      co_await gui.message_box(
          "We have already won the War of Independence." );
      break;
    case ref_cannot_declare:
      co_await gui.message_box(
          "The Royal Expeditionary Force cannot declare "
          "independence." );
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
        config_nation.nations[player.nation].country_name ),
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
  // TODO
  co_await ts.gui.message_box(
      "(signing of signature on declaration)" );
}

DeclarationResult declare_independence( IEngine& engine, SS& ss,
                                        TS& ts,
                                        Player& player ) {
  DeclarationResult res;
  CHECK( !is_ref( player.type ) );
  e_difficulty const difficulty =
      ss.as_const.settings.game_setup_options.difficulty;

  // Step: Set declaration status.
  player.revolution.status = e_revolution_status::declared;

  // Step: Add backup force.
  player.revolution.intervention_force =
      config_revolution.intervention_forces
          .unit_counts[difficulty];

  // Step: Add the REF player.
  e_player const ref_player_type =
      ref_player_for( player.nation );
  CHECK( ref_player_type != player.type );
  CHECK( !ss.players.players[ref_player_type].has_value() );
  Player& ref_player = add_new_player( ss, ref_player_type );
  ref_player.control = e_player_control::ai;
  ts.euro_agents().update(
      ref_player_type,
      create_euro_agent( engine, ss, ts.planes, ts.gui,
                         ref_player_type ) );

  // Step: Make sure that there is at least one Man-o-War if
  // there are any ref units to bring. The OG appears to increase
  // this from zero to one if it is zero.
  auto& force = player.revolution.expeditionary_force;
  if( force.regular + force.cavalry + force.artillery > 0 &&
      force.man_o_war == 0 )
    force.man_o_war = 1;

  // Step: Eliminate all foreign units outside of colonies.
  vector<UnitId> destroy;
  for( auto const& [unit_id, p_state] : ss.units.euro_all() ) {
    Unit const& unit = p_state->unit;
    if( unit.player_type() == player.type ) continue;
    CHECK( !is_ref( unit.player_type() ),
           "found unexpected REF units for player {} when "
           "player {} declared independence.",
           unit.player_type(), player.type );
    SWITCH( p_state->ownership ) {
      CASE( colony ) {
        // Leave the units in colonies because we need to leave
        // the colonies intact.
        break;
      }
      default:
        destroy.push_back( unit.id() );
        break;
    }
  }
  destroy_units( ss, destroy );

  // Step: Mark foreign players as "withdrawn"; they no longer
  // get evolved, probably at the colony level either. The
  // players can't be deleted because their colonies persist.
  for( auto& [type, other_player] : ss.players.players ) {
    if( !other_player.has_value() ) continue;
    // Make sure we're not hitting either this human player or
    // their new ref player that we just created.
    if( other_player->nation == player.nation ) continue;
    other_player->control = e_player_control::withdrawn;
  }

  // Step: Destroy all units on the dock and seize all ships in
  // port and on the high seas.
  destroy.clear();
  for( auto const& [unit_id, p_state] : ss.units.euro_all() ) {
    Unit const& unit = p_state->unit;
    if( unit.player_type() != player.type ) continue;
    SWITCH( p_state->ownership ) {
      CASE( harbor ) {
        SWITCH( harbor.port_status ) {
          CASE( in_port ) {
            if( unit.desc().ship )
              ++res.seized_ships[unit.type()];
            break;
          }
          CASE( inbound ) {
            CHECK( unit.desc().ship );
            ++res.seized_ships[unit.type()];
            break;
          }
          CASE( outbound ) {
            CHECK( unit.desc().ship );
            ++res.seized_ships[unit.type()];
            break;
          }
        }
        destroy.push_back( unit_id );
        break;
      }
      default:
        break;
    }
  }
  destroy_units( ss, destroy );

  // Step: zero out crosses and bells.
  player.crosses = 0;
  player.bells   = 0;

  // Step: Reset founding fathers acquisition state.
  player.fathers.in_progress.reset();
  player.fathers.pool = {};

  // Step: Offboard any units on ships in colonies. This is not
  // only convenient for the player, but also makes it more
  // straightforward to deal with the promotion of units to con-
  // tinental units simpler on the next turn.
  vector<ColonyId> const colonies =
      ss.colonies.for_player( player.type );
  for( ColonyId const colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    vector<UnitId> const offboarded =
        offboard_units_on_ships( ss, ts, colony.location );
    if( !offboarded.empty() ) res.offboarded_units = true;
  }

  // Step: End the turn of the player.
  for( auto& [unit_id, p_state] : ss.units.euro_all() ) {
    Unit& unit = ss.units.unit_for( unit_id );
    if( unit.player_type() != player.type ) continue;
    unit.forfeight_mv_points();
  }

  return res;
}

wait<> declare_independence_ui_sequence_post(
    SSConst const&, TS& ts, Player const&,
    DeclarationResult const& decl ) {
  co_await ts.gui.message_box(
      "Continental Congress signs [Declaration of "
      "Independence]! (more description of things that "
      "happen)" );

  for( auto const& [type, count] : decl.seized_ships ) {
    auto const& conf =
        config_unit_type.composition.unit_types[type];
    co_await ts.gui.message_box(
        "The King has seized [{} {}] on the High Seas!",
        base::int_to_string_literary( count ),
        ( count > 1 ) ? conf.name_plural : conf.name );
  }

  if( decl.offboarded_units )
    co_await ts.gui.message_box(
        "Units in the cargo of ships in our colonies have "
        "offboarded in order to help defend the colonies." );
}

e_turn_after_declaration post_declaration_turn(
    Player const& p ) {
  using enum e_turn_after_declaration;
  using enum e_revolution_status;
  if( !( p.revolution.status == declared ) ) return zero;
  if( !p.revolution.continental_army_mobilized ) return one;
  if( !p.revolution.gave_independence_war_hints ) return two;
  return done;
}

} // namespace rn
