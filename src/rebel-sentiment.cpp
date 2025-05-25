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
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

// There are messages that inform the player of when rebel senti-
// ment rises and falls, similar to the SoL messages. However,
// they are only shown when the total population is >= 4, and
// that includes all human units both in colonies, on the map, or
// on the dock, i.e. the `total_colonists` variable below.
int constexpr kMinColonistsForSentimentMsgs = 4;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
int unit_count_for_rebel_sentiment( SSConst const& ss,
                                    e_player const player ) {
  auto const& units = ss.units.euro_all();
  int n             = 0;
  for( auto const& [_, p_state] : units ) {
    Unit const& unit = p_state->unit;
    if( unit.player_type() != player ) continue;
    if( !is_unit_a_colonist( unit.type() ) ) continue;
    ++n;
  }
  return n;
}

int updated_rebel_sentiment( SSConst const& ss,
                             Player const& player ) {
  double fractional_rebels      = 0.0;
  int total_colonies_population = 0;
  for( auto const& [_, colony] : ss.colonies.all() ) {
    if( colony.player != player.type ) continue;
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
      unit_count_for_rebel_sentiment( ss, player.type );
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
    int const new_sentiment ) {
  // These should be done in order of least expensive to most ex-
  // pensive so that we do the expensive stuff less often.
  if( player.revolution.status >= e_revolution_status::declared )
    return false;
  if( unit_count_for_rebel_sentiment( ss, player.type ) <
      kMinColonistsForSentimentMsgs )
    return false;
  int const report_from =
      player.revolution.last_reported_rebel_sentiment;
  int const report_to       = new_sentiment;
  int constexpr kBucketSize = 10;
  auto const bucket         = []( int const n ) {
    return n / kBucketSize;
  };
  if( bucket( report_to ) == bucket( report_from ) )
    return false;
  // This is so that it doesn't report when fluctuating around a
  // bucket boundary.
  bool const decreasing    = report_to < report_from;
  int const jump_magnitude = abs( report_to - report_from );
  if( decreasing && jump_magnitude < kBucketSize / 2 )
    return false;
  return true;
}

wait<> show_rebel_sentiment_change_report(
    Player& player, IEuroMind& mind,
    RebelSentimentChangeReport const& report ) {
  auto const& country =
      config_nation.nations[nation_for( mind.player_type() )]
          .country_name;
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
  player.revolution.last_reported_rebel_sentiment = report.nova;
}

int required_rebel_sentiment_for_declaration(
    SSConst const& ss ) {
  return config_revolution.declaration
      .human_required_rebel_sentiment_percent
          [ss.settings.game_setup_options.difficulty]
      .percent;
}

} // namespace rn
