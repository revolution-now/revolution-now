/****************************************************************
**market.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Manipulates/evolves market prices.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "market.rds.hpp"

// Revolution Now
#include "ts.hpp"
#include "wait.hpp"

// ss
#include "ss/commodity.rds.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct Player;
struct SS;
struct SSConst;

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity );

// The prices are in hundreds.
int ask_from_bid( e_commodity type, int bid );

// The sign of the quantity in the commodity must be >= 0.
Invoice transaction_invoice( SSConst const& ss,
                             Player const&  player,
                             Commodity      comm,
                             e_transaction  transaction_type );

void apply_invoice( SS& ss, Player& player,
                    Invoice const& invoice );

// Just displays a message saying how and in which country the
// eprice changed.
wait<> display_price_change_notification(
    TS& ts, Player const& player, e_commodity commodity,
    int const price_change );

// This will evolve the european volumes for a single commodity,
// since in all of the models used here, they can be evolved in-
// dependently, and also sometimes in the game they need to be
// evolved independently.
void evolve_market_commodity( Player&     player,
                              e_commodity commodity );

bool is_in_processed_goods_price_group( e_commodity type );

} // namespace rn
