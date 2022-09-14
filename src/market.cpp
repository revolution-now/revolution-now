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
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

// TODO: for the default price model:
//
//   - Each turn, the attrition is applied first before the game
//     considers price changes.

int with_volatility( int what, int volatility ) {
  if( volatility >= 0 )
    return what * ( 1 << volatility );
  else
    return what * ( 1 >> ( -volatility ) );
}

} // namespace

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

Invoice transaction_invoice( SSConst const& ss,
                             Player const&  player,
                             Commodity      transacted,
                             e_transaction  transaction_type ) {
  CHECK( !is_in_processed_goods_price_group( transacted.type ) );
  CommodityPrice const prices =
      market_price( player, transacted.type );
  if( transaction_type == e_transaction::buy )
    // Make the quantity so that its sign reflects the sign of
    // the change in volume from the perspective of europe, since
    // that makes the below calculations easier.
    transacted.quantity = -transacted.quantity;
  int const         quantity  = transacted.quantity;
  e_commodity const comm_type = transacted.type;
  auto const&       item_config =
      config_market.price_behavior[comm_type];
  int const volatility = item_config.model_parameters.volatility;
  auto const& difficulty_modifiers =
      config_market.difficulty_modifiers[ss.settings.difficulty];
  MarketItem const player_market_item =
      player.old_world.market.commodities[comm_type];
  int const price = ( transaction_type == e_transaction::buy )
                        ? prices.ask
                        : prices.bid;

  Invoice res;
  res.what = transacted;

  // 1. Player money adjustment.
  res.money_delta_before_taxes = price * transacted.quantity;
  res.tax_rate                 = player.old_world.taxes.tax_rate;
  if( transaction_type == e_transaction::sell ) {
    CHECK_GE( res.money_delta_before_taxes, 0 );
    CHECK( res.money_delta_before_taxes % 100 == 0 );
    // Rounding is not an issue here because the amount received
    // will always be a multiple of 100, since bid/ask prices in
    // the game are always so.
    res.tax_amount =
        res.tax_rate * ( res.money_delta_before_taxes / 100 );
  }
  res.money_delta_final =
      res.money_delta_before_taxes - res.tax_amount;

  // 2. Change player-traded volume (recall that this is defined
  // as the volume from the european perspective).
  res.player_volume_delta = quantity;

  // 3. Change the intrinsic volumes.
  double base_volume_change =
      with_volatility( quantity, volatility );
  base_volume_change *=
      player.human
          ? difficulty_modifiers.human_traffic_volume_scale
          : difficulty_modifiers.non_human_traffic_volume_scale;

  for( e_nation nation : refl::enum_values<e_nation> ) {
    res.intrinsic_volume_delta[nation] = 0;
    maybe<Player const&> some_player =
        ss.players.players[nation];
    if( !some_player.has_value() ) continue;
    double const volume_change =
        base_volume_change *
        config_market.nation_advantage[nation].buy_volume_scale;
    // Here we want to round toward zero.
    res.intrinsic_volume_delta[nation] = int( volume_change );
  }

  // 4. Evolve the player's price and intrinsic volume for the
  // transacted commodity (only for player). Note that we need to
  // consider both rises and falls here, since the current in-
  // trinsic volume could have started off at any value.
  int const fall_threshold =
      item_config.model_parameters.fall * 100;
  int const rise_threshold =
      -item_config.model_parameters.rise * 100;
  int const new_player_volume =
      player_market_item.intrinsic_volume +
      res.intrinsic_volume_delta[player.nation];
  if( new_player_volume >= fall_threshold ) {
    // Price drop.
    res.intrinsic_volume_delta[player.nation] -= fall_threshold;
    int const new_price =
        std::max( player_market_item.bid_price - 1,
                  item_config.price_limits.bid_price_min );
    res.price_change = new_price - player_market_item.bid_price;
  } else if( new_player_volume <= rise_threshold ) {
    // Price increase.
    res.intrinsic_volume_delta[player.nation] -= rise_threshold;
    int const new_price =
        std::min( player_market_item.bid_price + 1,
                  item_config.price_limits.bid_price_max );
    res.price_change = new_price - player_market_item.bid_price;
  }
  CHECK_GE( res.price_change, -1 );
  CHECK_LE( res.price_change, 1 );
  return res;
}

void apply_invoice( SS& ss, Player& player,
                    Invoice const& invoice ) {
  player.money += invoice.money_delta_final;
  player.old_world.market.commodities[invoice.what.type]
      .player_traded_volume += invoice.player_volume_delta;
  for( e_nation nation : refl::enum_values<e_nation> ) {
    maybe<Player&> some_player = ss.players.players[nation];
    if( !some_player.has_value() ) continue;
    some_player->old_world.market.commodities[invoice.what.type]
        .intrinsic_volume +=
        invoice.intrinsic_volume_delta[nation];
  }
  player.old_world.market.commodities[invoice.what.type]
      .bid_price += invoice.price_change;
}

wait<> display_price_change_notification(
    TS& ts, Player const& player, e_commodity commodity,
    int const price_change ) {
  string const country_name =
      nation_obj( player.nation ).country_name;
  string const verb = ( price_change > 0 ) ? "risen" : "fallen";
  CommodityPrice const prices =
      market_price( player, commodity );
  string const msg =
      fmt::format( "The price of @[H]{}@[] in {} has {} to {}.",
                   commodity, country_name, verb, prices.bid );
  co_await ts.gui.message_box( msg );
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
