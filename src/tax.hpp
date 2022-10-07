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
// each choice that the player makes.
TaxUpdateComputation compute_tax_change( SSConst const& ss,
                                         TS&            ts,
                                         Player const&  player );

// This function will take a tax change proposal (which gives
// possible outcomes) and translate it to a final result that in-
// dicates the changes that actually need to be made, and in gen-
// eral this requires UI routines and player input.
wait<TaxChangeResult_t> prompt_for_tax_change_result(
    SSConst const& ss, TS& ts, Player& player,
    TaxChangeProposal_t const& update );

// Takes the TaxChangeResult_t object and applies any changes
// that it requires.
void apply_tax_result( SS& ss, Player& player,
                       int next_tax_event_turn,
                       TaxChangeResult_t const& change );

// This runs through the entire routine for a human player:
// checks for tax events, possibly prompts the player, and then
// makes any changes if necessary.
wait<> start_of_turn_tax_check( SS& ss, TS& ts, Player& player );

} // namespace rn
