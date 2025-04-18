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
struct SettingsState;
struct SS;
struct TS;
struct SSConst;

enum class e_nation;

/****************************************************************
** Evolution of Rebel Sentiment.
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

/****************************************************************
** Rebel Sentiment in Continental Congress Report.
*****************************************************************/
// This provides the rebel sentiment info for the Continental
// Congress report.
RebelSentimentReport rebel_sentiment_report_for_cc_report(
    SSConst const& ss, Player const& player );

/****************************************************************
** War of Succession.
*****************************************************************/
bool should_do_war_of_succession( SSConst const& ss,
                                  Player const& player );

WarOfSuccession do_war_of_succession( SS& ss,
                                      e_nation with_rebels );

wait<> do_war_of_succession_ui_seq(
    TS& ts, WarOfSuccession const& succession );

} // namespace rn
