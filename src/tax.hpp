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

// rds
#include "tax.rds.hpp"

// Revolution Now
#include "maybe.hpp"
#include "wait.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

struct IAgent;
struct IGui;
struct IRand;
struct Player;
struct SS;
struct SSConst;
struct TerrainConnectivity;

// Computes any changes that need to be made to the player's tax
// state and if there is a tax event this turn. If there is a tax
// event it will provide all of the precomputed results of it for
// each choice that the player makes.
TaxUpdateComputation compute_tax_change(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    IRand& rand, Player const& player );

// This function will take a tax change proposal (which gives
// possible outcomes) and translate it to a final result that in-
// dicates the changes that actually need to be made, and in gen-
// eral this requires UI routines and player input.
wait<TaxChangeResult> prompt_for_tax_change_result(
    SS& ss, IRand& rand, Player& player, IAgent& agent,
    TaxChangeProposal const& proposal );

// Takes the TaxChangeResult object and applies any changes
// that it requires.
void apply_tax_result( SS& ss, Player& player,
                       int next_tax_event_turn,
                       TaxChangeResult const& change );

// This runs through the entire routine for a human player:
// checks for tax events, possibly prompts the player, and then
// makes any changes if necessary.
wait<> start_of_turn_tax_check(
    SS& ss, IRand& rand, TerrainConnectivity const& connectivity,
    Player& player, IAgent& agent );

// If this commodity were currently boycotted then how much back-
// taxes would the player have to pay to remove it?
int back_tax_for_boycotted_commodity( SSConst const& ss,
                                      Player const& player,
                                      e_commodity type );

// This will run through the UI routine that happens when a
// player tries to trade a boycotted commodity. It will present
// them with the back tax amount and ask if they want to pay it.
// The commodity should be boycotted here otherwise it will
// check-fail. Returns the new boycott status (for convenience;
// if the status changed, the state will have been updated).
wait<base::NoDiscard<bool>> try_trade_boycotted_commodity(
    SS& ss, IGui& gui, Player& player, e_commodity type,
    int back_taxes );

} // namespace rn
