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
#include "revolution.rds.hpp"
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
  double fractional_rebels    = 0.0;
  int total_colony_population = 0;
  for( auto const& [_, colony] : ss.colonies.all() ) {
    ColonySonsOfLiberty const sol =
        compute_colony_sons_of_liberty( player, colony );
    int const population = colony_population( colony );
    total_colony_population += population;
    fractional_rebels +=
        double( sol.sol_integral_percent ) * population / 100.0;
  }
  int const sentiment =
      int( 100 * fractional_rebels / total_colony_population );
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
  int total_units =
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
  if( player.revolution.status >= e_revolution_status::declared )
    return false;
  auto const bucket     = []( int const n ) { return n / 10; };
  int const nova_bucket = bucket( report.nova );
  int const prev_bucket = bucket( report.prev );
  if( nova_bucket == prev_bucket ) return false;
  int const unit_count =
      unit_count_for_rebel_sentiment( ss, player.nation );
  if( unit_count < 4 ) return false;
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
  else
    co_await mind.message_box(
        "[Tory] sentiment is on the rise your Excellency.  Only "
        "[{}%] of the population now supports the idea of "
        "independence from {}.",
        report.nova, country );
}

} // namespace rn
