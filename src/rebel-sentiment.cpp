/****************************************************************
**rebel-sentiment.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-22.
*
* Description: Things related to Rebel Sentiment.
*
*****************************************************************/
#include "rebel-sentiment.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "ieuro-mind.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "sons-of-liberty.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"
#include "visibility.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/revolution.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/events.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rg = std::ranges;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_count;
using ::refl::enum_map;
using ::refl::enum_values;

// There are messages that inform the player of when rebel senti-
// ment rises and falls, similar to the SoL messages. However,
// they are only shown when the total population is >= 4, and
// that includes all human units both in colonies, on the map, or
// on the dock, i.e. the `total_colonists` variable below.
int constexpr kMinColonistsForSentimentMsgs = 4;

int unit_count_for_rebel_sentiment( SSConst const& ss,
                                    e_nation const nation ) {
  auto const& units = ss.units.euro_all();
  int n             = 0;
  for( auto const& [_, p_state] : units ) {
    Unit const& unit = p_state->unit;
    if( unit.nation() != nation ) continue;
    if( !is_unit_a_colonist( unit.type() ) ) continue;
    ++n;
  }
  return n;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
int updated_rebel_sentiment( SSConst const& ss,
                             Player const& player ) {
  double fractional_rebels      = 0.0;
  int total_colonies_population = 0;
  for( auto const& [_, colony] : ss.colonies.all() ) {
    if( colony.nation != player.nation ) continue;
    ColonySonsOfLiberty const sol =
        compute_colony_sons_of_liberty( player, colony );
    int const population = colony_population( colony );
    total_colonies_population += population;
    fractional_rebels +=
        double( sol.sol_integral_percent ) * population / 100.0;
  }
  if( total_colonies_population == 0 ) return 0;
  int const sentiment =
      int( 100 * fractional_rebels / total_colonies_population );
  CHECK_GE( sentiment, 0 );
  CHECK_LE( sentiment, 100 );
  return sentiment;
}

[[nodiscard]] RebelSentimentChangeReport update_rebel_sentiment(
    Player& player, int const updated ) {
  RebelSentimentChangeReport report;
  report.prev = player.revolution.rebel_sentiment;
  report.nova = updated;
  player.revolution.rebel_sentiment = report.nova;
  return report;
}

RebelSentimentReport rebel_sentiment_report_for_cc_report(
    SSConst const& ss, Player const& player ) {
  int const rebel_sentiment = player.revolution.rebel_sentiment;
  int const total_units =
      unit_count_for_rebel_sentiment( ss, player.nation );
  int const rebels =
      int( double( rebel_sentiment ) * total_units / 100.0 );
  int const tories = total_units - rebels;
  CHECK_GE( rebels, 0 );
  CHECK_GE( tories, 0 );
  CHECK_EQ( rebels + tories, total_units );
  return { .rebel_sentiment = rebel_sentiment,
           .rebels          = rebels,
           .tories          = tories };
}

bool should_show_rebel_sentiment_report(
    SSConst const& ss, Player const& player,
    RebelSentimentChangeReport const& report ) {
  auto const bucket = []( int const n ) { return n / 10; };
  // These should be done in order of least expensive to most ex-
  // pensive so that we do the expensive stuff less often.
  if( player.revolution.status >= e_revolution_status::declared )
    return false;
  if( bucket( report.nova ) == bucket( report.prev ) )
    return false;
  if( unit_count_for_rebel_sentiment( ss, player.nation ) <
      kMinColonistsForSentimentMsgs )
    return false;
  return true;
}

wait<> show_rebel_sentiment_change_report(
    IEuroMind& mind, RebelSentimentChangeReport const& report ) {
  auto const& country =
      config_nation.nations[mind.nation()].country_name;
  if( report.nova > report.prev )
    co_await mind.message_box(
        "[Rebel] sentiment is on the rise, Your Excellency!  "
        "[{}%] of the population now supports the idea of "
        "independence from {}.",
        report.nova, country );
  else if( report.nova < report.prev && report.nova > 0 )
    co_await mind.message_box(
        "[Tory] sentiment is on the rise, Your Excellency.  "
        "Only [{}%] of the population now supports the idea of "
        "independence from {}.",
        report.nova, country );
  else if( report.nova < report.prev && report.nova == 0 )
    co_await mind.message_box(
        "[Tory] sentiment is on the rise, Your Excellency.  "
        "None of the population supports the idea of "
        "independence from {}.",
        country );
}

int required_rebel_sentiment_for_declaration(
    SSConst const& ss ) {
  return config_revolution.declaration
      .human_required_rebel_sentiment_percent
          [ss.settings.game_setup_options.difficulty]
      .percent;
}

bool should_do_war_of_succession( SSConst const& ss,
                                  Player const& player ) {
  // NOTE: there a corresponding config option, but we don't
  // check that directly because that is intended to be a default
  // value for the new-game setting that we actually check.
  if( !ss.settings.game_setup_options.enable_war_of_succession )
    return false;
  // This human check is very important not only because the OG
  // does not trigger the war of succession in response to AI
  // player sentiment, but also because if this is an AI player
  // then we risk eliminating the player when it is their turn,
  // which is probably not a good thing.
  if( !player.human ) return false;
  if( ss.events.war_of_succession_done ) return false;
  auto const [player_count, human_count] = [&] {
    int player_count = 0;
    int human_count  = 0;
    for( auto const& [nation, other_player] :
         ss.players.players ) {
      if( !other_player.has_value() ) continue;
      if( !other_player->human &&
          other_player->revolution.status ==
              e_revolution_status::won )
        // AI player has been granted independence.
        continue;
      ++player_count;
      if( other_player->human ) ++human_count;
    }
    return pair{ player_count, human_count };
  }();
  if( player_count < 4 ) return false;
  if( human_count != 1 )
    // If we only have AI players then we're not going to be
    // fighting a war of independence, so no need for the war of
    // succession. Likewise, if there are multiple human players,
    // it'd probably be strange to have a war of succession,
    // since it is already a non traditional mode of gameplay.
    return false;
  int const required_sentiment =
      required_rebel_sentiment_for_declaration( ss );
  if( player.revolution.rebel_sentiment < required_sentiment )
    return false;
  return true;
}

WarOfSuccessionNations select_nations_for_war_of_succession(
    SSConst const& ss ) {
  vector<e_nation> ai_nations;
  ai_nations.reserve( enum_count<e_nation> );
  enum_map<e_nation, int> size_metric;
  for( e_nation const nation : enum_values<e_nation> ) {
    auto const& player = ss.players.players[nation];
    if( !player.has_value() ) continue;
    if( player->human ) continue;
    ai_nations.push_back( nation );
    int const population =
        unit_count_for_rebel_sentiment( ss, nation );
    int const colony_count =
        ss.colonies.for_nation( nation ).size();
    // This appears to be roughly what the OG does. There could
    // be further checks or other slight differences, but it
    // doesn't seem important to get it exactly the same, and
    // it's a bit tricky to determine empirically.
    size_metric[nation] = population + colony_count;
  }
  stable_sort( ai_nations.begin(), ai_nations.end(),
               [&]( e_nation const l, e_nation const r ) {
                 return size_metric[l] < size_metric[r];
               } );
  // The call to should_do_war_of_succession that we should have
  // done before calling this method should have ensured that
  // this won't happen.
  CHECK_GE( ssize( ai_nations ), 2 );
  e_nation const smallest        = ai_nations[0];
  e_nation const second_smallest = ai_nations[1];
  return WarOfSuccessionNations{ .withdraws = smallest,
                                 .receives  = second_smallest };
}

WarOfSuccessionPlan war_of_succession_plan(
    SSConst const& ss, WarOfSuccessionNations const& nations ) {
  WarOfSuccessionPlan plan;
  plan.nations = nations;

  unordered_map<UnitId, UnitState::euro const*> const& euros =
      ss.units.euro_all();
  for( auto const& [unit_id, p_state] : euros ) {
    Unit const& unit = p_state->unit;
    if( unit.nation() != nations.withdraws ) continue;
    SWITCH( p_state->ownership ) {
      CASE( free ) { SHOULD_NOT_BE_HERE; }
      CASE( cargo ) {
        // Cargo units will get reassigned if/when what is
        // holding them gets reassigned (or they could be de-
        // stroyed if they are contained in a ship that is in
        // port, which do not get reassigned).
        break;
      }
      CASE( colony ) {
        // We don't reassign units working in colonies here be-
        // cause they will be reassigned along with the colony
        // further below.
        break;
      }
      CASE( dwelling ) {
        plan.reassign_units.push_back( unit_id );
        // This is so that the missionary cross color gets up-
        // dated on the dwelling.
        point const tile = ss.natives.coord_for( dwelling.id );
        plan.update_fog_squares.push_back( tile );
        break;
      }
      CASE( world ) {
        plan.reassign_units.push_back( unit_id );
        break;
      }
      CASE( harbor ) {
        SWITCH( harbor.port_status ) {
          CASE( in_port ) {
            plan.remove_units.push_back( unit_id );
            break;
          }
          CASE( inbound ) {
            plan.reassign_units.push_back( unit_id );
            break;
          }
          CASE( outbound ) {
            plan.reassign_units.push_back( unit_id );
            break;
          }
        }
        break;
      }
    }
  }

  for( auto const& [colony_id, colony] : ss.colonies.all() ) {
    if( colony.nation != nations.withdraws ) continue;
    plan.reassign_colonies.push_back( colony_id );
    plan.update_fog_squares.push_back( colony.location );
  }

  rg::sort( plan.update_fog_squares,
            []( auto const l, auto const r ) {
              if( l.y != r.y ) return l.y < r.y;
              return l.x < r.x;
            } );
  rg::unique( plan.update_fog_squares );

  // Need determinism for unit tests.
  rg::sort( plan.remove_units );
  rg::sort( plan.reassign_units );
  rg::sort( plan.reassign_colonies );

  return plan;
}

void do_war_of_succession( SS& ss, TS& ts, Player const& player,
                           WarOfSuccessionPlan const& plan ) {
  CHECK_NEQ( player.nation, plan.nations.withdraws );
  CHECK_NEQ( player.nation, plan.nations.receives );
  for( UnitId const unit_id : plan.remove_units )
    // The unit could already have been deleted if e.g. it was in
    // the cargo of a ship and we deleted the ship first.
    if( ss.units.exists( unit_id ) )
      UnitOwnershipChanger( ss, unit_id ).destroy();

  for( UnitId const unit_id : plan.reassign_units ) {
    // The unit could have been deleted if e.g. it was a unit in
    // the cargo of a ship in the harbor.
    if( !ss.units.exists( unit_id ) ) continue;
    Unit& unit = ss.units.unit_for( unit_id );
    change_unit_nation( ss, ts, unit, plan.nations.receives );
  }

  for( ColonyId const colony_id : plan.reassign_colonies ) {
    Colony& colony = ss.colonies.colony_for( colony_id );
    change_colony_nation( ss, ts, colony,
                          plan.nations.receives );
    // The OG appears to reduce SoL to zero for the acquired
    // player. This is likely because then the merger would risk
    // causing a large bump to the total number of rebels in the
    // acquiring nation and thus could risk immediately causing
    // them to be granted independence, which would lead to a
    // strange player experience. This way, the AI nations are no
    // further along in that process than they were before.
    reset_colony_sons_of_liberty( colony );
    // TODO: not sure yet how we'll be handling evolution of the
    // AI colonies, but this lowering of SoL to zero (under stan-
    // dard rules) could cause a drop in productivity and thus
    // starvation in the colony, so we may need to deal with that
    // here.
  }

  vector<Coord> const refresh_fogged = [&] {
    vector<Coord> res;
    res.reserve( plan.update_fog_squares.size() );
    VisibilityForNation const viz( ss, player.nation );
    for( Coord const tile : plan.update_fog_squares )
      if( viz.visible( tile ) == e_tile_visibility::fogged )
        res.push_back( tile );
    return res;
  }();
  // This is not perfect because it will refresh the contents of
  // the entire tile, not limited to the nation change. E.g. for
  // a colony it will not only update the nation but will also
  // update the population and the fort type. But this makes
  // things simpler, and probably doesn't make much of a differ-
  // ence anyway.
  ts.map_updater().make_squares_visible( player.nation,
                                         refresh_fogged );
  ts.map_updater().make_squares_fogged( player.nation,
                                        refresh_fogged );

  ss.players.players[plan.nations.withdraws].reset();

  ss.events.war_of_succession_done = true;
}

wait<> do_war_of_succession_ui_seq(
    TS& ts, WarOfSuccessionPlan const& plan ) {
  // TODO: add more historical context to this message.
  string const msg = format(
      "The War of the Spanish Succession has come to an end in "
      "Europe! All property and territory owned by the [{}] has "
      "been ceded to the [{}].  As a result, the [{}] have "
      "withdrawn from the New World.",
      config_nation.nations[plan.nations.withdraws]
          .possessive_pre_declaration,
      config_nation.nations[plan.nations.receives]
          .possessive_pre_declaration,
      config_nation.nations[plan.nations.withdraws]
          .possessive_pre_declaration );
  co_await ts.gui.message_box( msg );
}

} // namespace rn
