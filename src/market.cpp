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
#include "price-group.hpp"
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

Invoice transaction_invoice_default_model(
    SSConst const& ss, Player const& player,
    Commodity transacted, e_transaction transaction_type ) {
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
  PlayerMarketItem const player_market_item =
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
    // Rounding is not an issue here because the amount received
    // will always be a multiple of 100, since bid/ask prices in
    // the game are always so.
    res.tax_amount =
        int( res.tax_rate *
             ( res.money_delta_before_taxes / 100.0 ) );
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

Invoice transaction_invoice_processed_group_model(
    SSConst const& ss, Player const& player,
    Commodity transacted, e_transaction transaction_type ) {
  CHECK( is_in_processed_goods_price_group( transacted.type ) );
  CommodityPrice const prices =
      market_price( player, transacted.type );

  Invoice invoice;
  invoice.what = transacted;

  // 1. Player money adjustment (note sign).
  int const price = ( transaction_type == e_transaction::buy )
                        ? -prices.ask
                        : prices.bid;
  // FIXME: dedupe this.
  invoice.money_delta_before_taxes = price * transacted.quantity;
  invoice.tax_rate = player.old_world.taxes.tax_rate;
  if( transaction_type == e_transaction::sell ) {
    CHECK_GE( invoice.money_delta_before_taxes, 0 );
    invoice.tax_amount =
        int( invoice.tax_rate *
             ( invoice.money_delta_before_taxes / 100.0 ) );
  } else {
    CHECK_LE( invoice.money_delta_before_taxes, 0 );
  }
  invoice.money_delta_final =
      invoice.money_delta_before_taxes - invoice.tax_amount;

  // 2. Change player-traded volume (recall that this is defined
  // as the volume from the european perspective).
  invoice.player_volume_delta =
      ( transaction_type == e_transaction::buy )
          ? -transacted.quantity
          : transacted.quantity;

  // 2. Evolve the global intrinsic volumes.

  // The sum of the player-traded volume across all players for a
  // single commodity.
  auto total_volume = [&]( e_commodity c ) {
    int sum = 0;
    for( auto const& [nation, player] : ss.players.players )
      if( player.has_value() )
        sum += player->old_world.market.commodities[c]
                   .player_traded_volume;
    return sum;
  };

  ProcessedGoodsPriceGroupConfig config =
      default_processed_goods_price_group_config();

  config.dutch = ( player.nation == e_nation::dutch );

  for( e_processed_good good :
       refl::enum_values<e_processed_good> ) {
    e_commodity const comm = to_commodity( good );
    config.starting_intrinsic_volumes[good] =
        ss.players.global_market_state.commodities[comm]
            .intrinsic_volume;
    config.starting_traded_volumes[good] = total_volume( comm );
  }

  // Create price group object.
  ProcessedGoodsPriceGroup group( config );

  UNWRAP_CHECK( processed_type,
                from_commodity( transacted.type ) );
  if( transaction_type == e_transaction::buy )
    group.buy( processed_type, transacted.quantity );
  else
    group.sell( processed_type, transacted.quantity );

  for( e_processed_good good :
       refl::enum_values<e_processed_good> ) {
    e_commodity const comm = to_commodity( good );
    invoice.global_intrinsic_volume_deltas[comm] =
        group.intrinsic_volume( good ) -
        ss.players.global_market_state.commodities[comm]
            .intrinsic_volume;
  }

  // 3. Evolve the player's price, but only for the transacted
  // commodity and only for player making the transaction.
  int const rise_fall =
      config_market.processed_goods_model.rise_fall;
  int const volatility =
      config_market.processed_goods_model.volatility;
  // The only place that the volatility and fall should be used
  // is together in this manner.
  double volatility_push = ( transacted.quantity / 100.0 ) *
                           double( 1 << volatility ) / rise_fall;
  CHECK_GE( volatility_push, 0 );
  if( transaction_type == e_transaction::sell )
    volatility_push = -volatility_push;
  ProcessedGoodsPriceGroup::Map const equilibrium_prices =
      group.equilibrium_prices();
  // Need to clamp at both steps, since it is observed in the OG
  // that when the volatility_push becomes unity that it can
  // overtake the eq_push. And then we need to clamp the net_push
  // because in general in the OG the price can never move more
  // than one unit per sale.
  int const eq_push = clamp(
      equilibrium_prices[processed_type] - prices.ask, -1, 1 );
  // The round is important here since otherwise any pushes that
  // are between -1 and 1 would get ironed out to zero, and that
  // changes the behavior because in the OG the volatility is 1,
  // meaning that the volatility push when purchasing 100 of
  // something will be +/.5, and so if that's in the opposite di-
  // rection as the eq_push then the net push will be +/.5 and
  // nothing will happen. By rounding, we accept this .5 as
  // enough to push the price, and this seems to more closey
  // mirror the OG's behavior (though not sure the exact formula
  // it uses for deciding how to push the price).
  int const net_push =
      lround( clamp( eq_push + volatility_push, -1.0, 1.0 ) );
  invoice.price_change = net_push;
  CHECK_GE( invoice.price_change, -1 );
  CHECK_LE( invoice.price_change, 1 );
  return invoice;
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
  if( is_in_processed_goods_price_group( transacted.type ) )
    return transaction_invoice_processed_group_model(
        ss, player, transacted, transaction_type );
  else
    return transaction_invoice_default_model(
        ss, player, transacted, transaction_type );
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
  for( auto const& [comm, delta] :
       invoice.global_intrinsic_volume_deltas )
    ss.players.global_market_state.commodities[comm]
        .intrinsic_volume += delta;
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
