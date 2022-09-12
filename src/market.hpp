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

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity );

// The prices are in hundreds.
int ask_from_bid( e_commodity type, int bid );

// The cost that the player would have to pay if the given com-
// modity type and quantity were bought all in one shot. Note
// that it only really makes sense to call this with a quantity
// that is <= 100 because the game does not allow buying more
// than that at once, that way the market prices can adjust after
// each purchase. This will not actually make the purchase, it
// just describes the results. Note that this will ignore any
// boycott status.
PurchaseInvoice purchase_invoice( Player const& player,
                                  Commodity     comm );

// A breakdown of the results of selling the given commodity type
// and quantity all in one shot. Note that it only really makes
// sense to call this with a quantity that is <= 100 because
// quantities larger than this can't be sold in one shot in the
// game in practice; the game puts a limit on 100 per transaction
// so that the market prices can respond to the sales. Note that
// this will ignore any boycott status. Note that this will not
// actually make the sale, it just describes the results (not in-
// cluding any subsequent price changes that might happen were it
// to occur).
SaleInvoice sale_invoice( Player const& player, Commodity comm );

bool is_in_processed_goods_price_group( e_commodity type );

// This method computes the "hidden" equilibrium prices that are
// not visible directly to the player. All of the commodities
// have equilibrium prices which determine how the actual (cur-
// rent) price moves each turn. The equilibrium prices are basi-
// cally a function of the volumes and starting prices, depending
// on the model, and as such they also move each turn.
//
// The reason this method does all of the commodities in bulk is
// in order to accomodate the commodities in price groups, which
// can only be computed together (we could do them separately but
// it would be less efficient).
refl::enum_map<e_commodity, CommodityPrice>
compute_equilibrium_prices( Player const& player );

// Given the current set of equilibrium prices, this will deter-
// mine if the current prices should move. If they do, they can
// do so by at most +1/-1 each time this function is called.
maybe<PriceChange> compute_price_change(
    Player const& player,
    refl::enum_map<e_commodity, CommodityPrice> const&
                equilibrium_prices,
    e_commodity type );

// Call this to actually affect a current price change.
void change_price( Player& player, e_commodity type,
                   PriceChange price_change );

// This will actually make a purchase. This means that the com-
// modity and quantity described in the invoice will be pur-
// chased, and the relevant european volumes will be updated
// (where/when applicable). The caller is responsible for en-
// suring that the player has enough money; this function will
// not check or care. This may also cause the current price to
// change. If so, that will be returned.
maybe<PriceChange> buy_commodity_from_harbor(
    SS& ss, Player& player, PurchaseInvoice const& invoice );

// Similar to the above but for sales.
maybe<PriceChange> sell_commodity_from_harbor(
    SS& ss, Player& player, SaleInvoice const& invoice );

// Just displays a message saying how and in which country the
// eprice changed.
wait<> display_price_change_notification(
    TS& ts, Player const& player, e_commodity commodity,
    PriceChange const& price_change );

// This will evolve the european volumes for a single commodity,
// since in all of the models used here, they can be evolved in-
// dependently, and also sometimes in the game they need to be
// evolved independently.
void evolve_volume( Player& player, e_commodity commodity );

} // namespace rn
