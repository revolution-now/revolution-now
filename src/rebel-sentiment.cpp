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

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

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

bool should_do_war_of_succession( SSConst const&,
                                  Player const& ) {
  return false;
}

WarOfSuccession do_war_of_succession( SS&, e_nation const ) {
  WarOfSuccession res;
  return res;
}

wait<> do_war_of_succession_ui_seq( TS&,
                                    WarOfSuccession const& ) {
  co_return;
}

} // namespace rn
