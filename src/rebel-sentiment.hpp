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
struct IEuroAgent;
struct Player;
struct SSConst;

enum class e_player;

/****************************************************************
** Evolution of Rebel Sentiment.
*****************************************************************/
[[nodiscard]] int updated_rebel_sentiment(
    SSConst const& ss, Player const& player );

[[nodiscard]] RebelSentimentChangeReport update_rebel_sentiment(
    Player& player, int updated );

bool should_show_rebel_sentiment_report( SSConst const& ss,
                                         Player const& player,
                                         int new_sentiment );

wait<> show_rebel_sentiment_change_report(
    Player& player, IEuroAgent& agent,
    RebelSentimentChangeReport const& report );

int required_rebel_sentiment_for_declaration(
    SSConst const& ss );

int unit_count_for_rebel_sentiment( SSConst const& ss,
                                    e_player const player );

/****************************************************************
** Rebel Sentiment in Continental Congress Report.
*****************************************************************/
// This provides the rebel sentiment info for the Continental
// Congress report.
RebelSentimentReport rebel_sentiment_report_for_cc_report(
    SSConst const& ss, Player const& player );

} // namespace rn
