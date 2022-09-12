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

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "ts.hpp"

// config
#include "config/market.rds.hpp"
#include "config/nation.hpp"

// ss
#include "ss/player.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/to-str.hpp"

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

int ask_from_bid( e_commodity type, int bid ) {
  return bid + config_market.price_behavior[type]
                   .price_limits.bid_ask_spread;
}

PurchaseInvoice purchase_invoice( Player const& player,
                                  Commodity     comm ) {
  CommodityPrice const prices =
      market_price( player, comm.type );
  return PurchaseInvoice{ .purchased = comm,
                          .cost = prices.ask * comm.quantity };
}

SaleInvoice sale_invoice( Player const& player,
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

maybe<PriceChange> buy_commodity_from_harbor(
    SS&, Player& player, PurchaseInvoice const& invoice ) {
  int const         quantity = invoice.purchased.quantity;
  e_commodity const type     = invoice.purchased.type;
  player.money -= invoice.cost;
  MarketItem& market_item =
      player.old_world.market
          .commodities[invoice.purchased.type];
  market_item.unscaled_net_traded_volume -= quantity;
  // TODO: Apply dutch bonus.
  // TODO: Apply difficulty bonus.
  // TODO: Apply to other players' markets.
  market_item.scaled_net_traded_volume -= quantity;
  // market_item.intrinsic_volume = 0;
  // market_item.current_bid_price_in_hundreds = 0;
  refl::enum_map<e_commodity, CommodityPrice> const&
      equilibrium_prices = compute_equilibrium_prices( player );
  return compute_price_change( player, equilibrium_prices,
                               type );
}

maybe<PriceChange> sell_commodity_from_harbor(
    SS&, Player& player, SaleInvoice const& invoice ) {
  int const         quantity = invoice.sold.quantity;
  e_commodity const type     = invoice.sold.type;
  player.money += invoice.received_final;
  MarketItem& market_item =
      player.old_world.market.commodities[invoice.sold.type];
  market_item.unscaled_net_traded_volume += quantity;
  // TODO: Apply dutch bonus.
  // TODO: Apply difficulty bonus.
  // TODO: Apply to other players' markets.
  market_item.scaled_net_traded_volume += quantity;
  // market_item.intrinsic_volume = 0;
  // market_item.current_bid_price_in_hundreds = 0;
  refl::enum_map<e_commodity, CommodityPrice> const&
      equilibrium_prices = compute_equilibrium_prices( player );
  return compute_price_change( player, equilibrium_prices,
                               type );
}

refl::enum_map<e_commodity, CommodityPrice>
compute_equilibrium_prices( Player const& player ) {
  refl::enum_map<e_commodity, CommodityPrice> res;
  for( e_commodity type : refl::enum_values<e_commodity> ) {
    MarketItem const& market_item =
        player.old_world.market.commodities[type];
    auto& model =
        config_market.price_behavior[type].economic_model;
    int bid = market_item.starting_bid_price_in_hundreds;
    if( market_item.scaled_net_traded_volume > 0 )
      bid -= market_item.scaled_net_traded_volume / model.fall;
    else
      bid += -market_item.scaled_net_traded_volume / model.rise;
    bid = clamp( bid, 0, 19 );
    CommodityPrice eq_price{ .bid = bid,
                             .ask = ask_from_bid( type, bid ) };
    res[type] = eq_price;
  }
  return res;
}

maybe<PriceChange> compute_price_change(
    Player const& player,
    refl::enum_map<e_commodity, CommodityPrice> const&
                equilibrium_prices,
    e_commodity type ) {
  MarketItem const& market_item =
      player.old_world.market.commodities[type];
  int       eq_bid  = equilibrium_prices[type].bid;
  int const current = market_item.current_bid_price_in_hundreds;
  if( current < eq_bid )
    return PriceChange{ .from = current, .to = current + 1 };
  else if( current > eq_bid )
    return PriceChange{ .from = current, .to = current - 1 };
  return nothing;
}

wait<> display_price_change_notification(
    TS& ts, Player const& player, e_commodity commodity,
    PriceChange const& price_change ) {
  string const country_name =
      nation_obj( player.nation ).country_name;
  string const verb = ( price_change.from < price_change.to )
                          ? "risen"
                          : "fallen";
  string const msg  = fmt::format(
      "The price of @[H]{}@[] in {} has {} to {}.", commodity,
      country_name, verb, price_change.to );
  co_await ts.gui.message_box( msg );
}

void change_price( Player& player, e_commodity type,
                   PriceChange price_change ) {
  player.old_world.market.commodities[type]
      .current_bid_price_in_hundreds = price_change.to;
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
