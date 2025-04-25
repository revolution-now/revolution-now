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
#include "sons-of-liberty.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/revolution.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/events.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

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

bool should_do_war_of_succession( SSConst const& ss,
                                  Player const& player ) {
  // NOTE: there a corresponding config option, but we don't
  // check that directly because that is intended to be a default
  // value for the new-game setting that we actually check.
  if( !ss.settings.game_setup_options.enable_war_of_succession )
    return false;
  if( !player.human ) return false;
  if( ss.events.war_of_succession_done ) return false;
  auto const [player_count, human_count] = [&] {
    int player_count = 0;
    int human_count  = 0;
    for( auto const& [nation, other_player] :
         ss.players.players ) {
      if( !other_player.has_value() ) continue;
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
  if( player.revolution.rebel_sentiment <
      config_revolution.declaration
          .human_required_rebel_sentiment_percent
              [ss.settings.game_setup_options.difficulty]
          .percent )
    return false;
  return true;
}

WarOfSuccessionPlan select_nations_for_war_of_succession(
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
  return WarOfSuccessionPlan{ .withdraws = smallest,
                              .receives  = second_smallest };
}

WarOfSuccession do_war_of_succession(
    SS&, WarOfSuccessionPlan const& ) {
  WarOfSuccession res;
  return res;
}

wait<> do_war_of_succession_ui_seq( TS&,
                                    WarOfSuccession const& ) {
  co_return;
}

} // namespace rn
