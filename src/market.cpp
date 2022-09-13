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

// TODO: for normal price model:
//
//   - Each nation has their own volumes.
//   - Each turn, the attrition is applied first before the game
//     considers price changes.
//   - When a transaction is made, it changes the other volumes
//     of the other nations. The amount of this change depends on
//     difficulty level and the player's nation. The dutch will
//     have their intrinsic volume deltas scaled by 66% (whether
//     the trade was made by them or someone else), but only on
//     sells.
//   - When non-player nations buy/sell it seems to only affect
//     the volumes by 66%, regardless of difficulty level. But
//     when the human player buys/sells, it affects the volumes
//     (of all nations) by an amount that varies depending on
//     difficulty level (and which is also subject to the dutch
//     sale multiplier). Actually, experiments indicate that,
//     when volatility is non-zero (enabling the player to add
//     more than 100 to the intrinsic volume in each trade), some
//     of these multipliers only apply to the first 100 sold in a
//     transaction; not sure if this was intentional, but we will
//     not replicate that in this game, since it feels like it
//     could have been a bug and doesn't seem to make sense.
//
//         Difficulty         P  NP
//         ------------------------
//         Discoverer:      .66 .66
//         Explorer:        .83 .66
//         Conquistador:   1.00 .66
//         Governor:       1.16 .66
//         Viceroy:        1.33 .66
//
//   - When any nation buys, it affects all nations (including
//     the dutch the same way); but on selling, it affects the
//     dutch with only 66%, compounded with any other multipli-
//     ers.
//   - When there is no transaction activity, the nations' mar-
//     kets do not tend to equilibrate (apart from the usual at-
//     trition drift). This means that the only interaction be-
//     tween them is that transactions in one nation affect vol-
//     umes in the others.

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity ) {
  int const bid =
      player.old_world.market.commodities[commodity].bid_price;
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
  market_item.player_traded_volume -= quantity;
  // TODO: Apply dutch bonus.
  // TODO: Apply difficulty bonus.
  // TODO: Apply to other players' markets.
  market_item.intrinsic_volume -= quantity;
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
  market_item.player_traded_volume += quantity;
  // TODO: Apply dutch bonus.
  // TODO: Apply difficulty bonus.
  // TODO: Apply to other players' markets.
  market_item.intrinsic_volume += quantity;
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
        config_market.price_behavior[type].model_parameters;
    // TODO
    (void)model;
    int bid = market_item.bid_price;
    bid     = clamp( bid, 0, 19 );
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
  int const current = market_item.bid_price;
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
  player.old_world.market.commodities[type].bid_price =
      price_change.to;
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
