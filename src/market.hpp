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
#include "wait.hpp"

// ss
#include "ss/commodity.rds.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct Player;
struct SS;
struct SSConst;
struct TS;

/****************************************************************
** Public API
*****************************************************************/
bool is_in_processed_goods_price_group( e_commodity type );

PriceChange create_price_change( Player const& player,
                                 e_commodity   comm,
                                 int           price_change );

CommodityPrice market_price( Player const& player,
                             e_commodity   commodity );

// The prices are in hundreds.
int ask_from_bid( e_commodity type, int bid );

// The sign of the quantity in the commodity must be >= 0.
[[nodiscard]] Invoice transaction_invoice(
    SSConst const& ss, Player const& player, Commodity comm,
    e_transaction transaction_type );

void apply_invoice( SS& ss, Player& player,
                    Invoice const& invoice );

// Just displays a message saying how and in which country the
// eprice changed.
wait<> display_price_change_notification(
    TS& ts, Player const& player, PriceChange const& change );

// This will evolve non-price market state for the commodities
// that are in price gruops. This is done once at the start of
// each full turn (where in this case one "turn" is where all of
// the nations move), before any players get their turn, because
// the price group model goods use shared market state. Then, at
// the start of each player turn, the game will use this new
// state to check for a price move. Hence this function does not
// attempt to move prices.
void evolve_group_model_volumes( SS& ss );

// This is called once at the start of each player turn and it
// will evolve their prices (for all commodities) and will return
// any price changes that occurred.
[[nodiscard]] refl::enum_map<e_commodity, PriceChange>
evolve_player_prices( SSConst const& ss, Player& player );

} // namespace rn
