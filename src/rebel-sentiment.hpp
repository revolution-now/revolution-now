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
    int new_sentiment );

wait<> show_rebel_sentiment_change_report(
    Player& player, IEuroMind& mind,
    RebelSentimentChangeReport const& report );

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
int required_rebel_sentiment_for_declaration(
    SSConst const& ss );

bool should_do_war_of_succession( SSConst const& ss,
                                  Player const& player );

WarOfSuccessionNations select_nations_for_war_of_succession(
    SSConst const& ss );

WarOfSuccessionPlan war_of_succession_plan(
    SSConst const& ss, WarOfSuccessionNations const& nations );

void do_war_of_succession( SS& ss, TS& ts, Player const& player,
                           WarOfSuccessionPlan const& plan );

wait<> do_war_of_succession_ui_seq(
    TS& ts, WarOfSuccessionPlan const& plan );

} // namespace rn
