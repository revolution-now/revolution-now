/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Manipulates/evolves market prices.
*
*****************************************************************/
#include "market.hpp"

// config
#include "config/market.rds.hpp"

// ss
#include "ss/player.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

void linker_dont_discard_module_market();
void linker_dont_discard_module_market() {}

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity ) {
  int const bid = player.old_world.market.commodities[commodity]
                      .current_bid_price_in_hundreds;
  int const ask = bid + config_market.price_behavior[commodity]
                            .price_limits.bid_ask_spread;
  return CommodityPrice{ .bid = bid, .ask = ask };
}

PurchaseInvoice cost_to_buy( Player const& player,
                             Commodity     comm ) {
  CommodityPrice const prices =
      market_price( player, comm.type );
  return PurchaseInvoice{ .purchased = comm,
                          .cost = prices.ask * comm.quantity };
}

SaleInvoice sale_transaction( Player const& player,
                              Commodity     comm ) {
  CommodityPrice const prices =
      market_price( player, comm.type );
  SaleInvoice res;
  res.sold                  = comm;
  res.received_before_taxes = prices.bid * comm.quantity;
  res.tax_rate              = player.old_world.taxes.tax_rate;
  // Rounding is not an issue here because the amount received
  // will always be a multiple of 100, since bid/ask prices in
  // the game are always so.
  res.tax_amount =
      ( res.received_before_taxes / 100 ) * res.tax_rate;
  res.received_final =
      res.received_before_taxes - res.tax_amount;
  CHECK_GE( res.received_final, 0 );
  return res;
}

bool is_in_processed_goods_price_group( e_commodity type ) {
  switch( type ) {
    case e_commodity::rum:
    case e_commodity::cigars:
    case e_commodity::cloth:
    case e_commodity::coats: //
      return true;
    default: //
      return false;
  }
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// FIXME: temporary until we can expose config data to lua.
LUA_FN( starting_price_limits, lua::table, e_commodity comm ) {
  lua::table tbl = st.table.create();
  tbl["bid_price_start_min"] =
      config_market.price_behavior[comm]
          .price_limits.bid_price_start_min;
  tbl["bid_price_start_max"] =
      config_market.price_behavior[comm]
          .price_limits.bid_price_start_max;
  return tbl;
}

} // namespace
} // namespace rn
