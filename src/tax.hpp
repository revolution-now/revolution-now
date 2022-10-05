/****************************************************************
**tax.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Handles things related to tax increases and
*              boycotts.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "tax.rds.hpp"
#include "wait.hpp"

// Revolution Now
#include "maybe.hpp"

namespace rn {

struct Player;
struct SS;
struct SSConst;
struct TS;

// Computes any changes that need to be made to the player's tax
// state and if there is a tax event this turn. If there is a tax
// event it will provide all of the precomputed results of it for
// each choice that the player makes. If this function returns
// nothing then there is nothing further to do this turn. If it
// returns something then we need to call apply_tax_change with
// that object to actually make the changes and run through the
// UI routine.
maybe<TaxationUpdate> compute_tax_change( SSConst const& ss,
                                          Player const& player );

// Given a TaxationUpdate object this function will determine if
// the player needs to be prompted and, if so, will go through
// the UI sequence, and return whether they accept or reject the
// change. For anything other than the increase-or-party, this
// will just return "accept," since the player has no reason to
// ever reject the others. Technically, they could want to reject
// a tax decrease if they have Thomas Paine (causes bell produc-
// tion to go up with tax rate) but the OG doesn't do that and it
// would be kind of strange.
wait<e_tax_proposal_answer> prompt_player_for_tax_change(
    SSConst const& ss, TS& ts, Player const& player,
    TaxationUpdate const& update );

// Takes the TaxationUpdate object and applies any changes that
// it requires.
void apply_tax_change( SS& ss, TS& ts, Player& player,
                       TaxationUpdate const& update );

// This runs through the entire routine for a human player:
// checks for tax events, possibly prompts the player, and then
// makes any changes if necessary.
wait<> start_of_turn_tax_check( SS& ss, TS& ts, Player& player );

} // namespace rn
