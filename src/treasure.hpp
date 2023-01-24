/****************************************************************
**treasure.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-17.
*
* Description: Things related to treasure trains.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "treasure.rds.hpp"

// Revolution Now
#include "maybe.hpp"
#include "wait.hpp"

namespace rn {

struct Dwelling;
struct Player;
struct SS;
struct SSConst;
struct TS;
struct Unit;

// This is what computes the result of bringing a treasure unit
// to the harbor and reimbursing it. The result of this function
// must then be given to the "apply" function to affect it.
TreasureReceipt treasure_in_harbor_receipt(
    Player const& player, Unit const& treasure );

// Runs through the UI sequence that happens when a treasure
// train enters a colony. Basically the king will ask the player
// if they want to transport it and how much of a cut they would
// take, then the player chooses. If the player accepts the offer
// then the receipt will be returned, though no changes will be
// made; the receipt then has to be given to the "apply" function
// to take effect.
wait<maybe<TreasureReceipt>> treasure_enter_colony(
    SSConst const& ss, TS& ts, Player const& player,
    Unit const& treasure );

// This will actually delete the treasure unit and give the
// player the amount of money specified in the receipt.
void apply_treasure_reimbursement(
    SS& ss, Player& player, TreasureReceipt const& receipt );

// After the treasure has been reimbursed (regardless of method)
// call this to show the user how much they've received.
wait<> show_treasure_receipt( TS& ts, Player const& player,
                              TreasureReceipt const& receipt );

// Randomly determines whether a destroyed dwelling should yield
// a treasure and, if so, how much. Note that if the player has
// Cortes then this will always yield a treasure, and in a larger
// amount on average.
maybe<int> treasure_from_dwelling( SSConst const& ss, TS& ts,
                                   Player const&   player,
                                   Dwelling const& dwelling );

} // namespace rn
