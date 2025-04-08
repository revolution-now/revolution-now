/****************************************************************
**rebel-sentiment.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-22.
*
* Description: Things related to Rebel Sentiment.
*
*****************************************************************/
#pragma once

// rds
#include "rebel-sentiment.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct IEuroMind;
struct Player;
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] int updated_rebel_sentiment(
    SSConst const& ss, Player const& player );

[[nodiscard]] RebelSentimentChangeReport update_rebel_sentiment(
    Player& player, int updated );

bool should_show_rebel_sentiment_report(
    SSConst const& ss, Player const& player,
    RebelSentimentChangeReport const& report );

wait<> show_rebel_sentiment_change_report(
    IEuroMind& mind, RebelSentimentChangeReport const& report );

// This provides the rebel sentiment info for the Continental
// Congress report.
RebelSentimentReport rebel_sentiment_report_for_cc_report(
    SSConst const& ss, Player const& player );

} // namespace rn
