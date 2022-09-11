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

// ss
#include "ss/commodity.rds.hpp"

namespace rn {

struct Player;

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity );

// The cost that the player would have to pay if the given com-
// modity type and quantity were bought all in one shot. Note
// that it only really makes sense to call this with a quantity
// that is <= 100 because the game does not allow buying more
// than that at once, that way the market prices can adjust after
// each purchase. Note that this will ignore any boycott status.
int cost_to_buy( Player const& player, Commodity comm );

// A breakdown of the results of selling the given commodity type
// and quantity all in one shot. Note that it only really makes
// sense to call this with a quantity that is <= 100 because
// quantities larger than this can't be sold in one shot in the
// game in practice; the game puts a limit on 100 per transaction
// so that the market prices can respond to the sales. Note that
// this will ignore any boycott status.
SaleInvoice sale_transaction( Player const& player,
                              Commodity     comm );

bool is_in_price_group( e_commodity type );

} // namespace rn
