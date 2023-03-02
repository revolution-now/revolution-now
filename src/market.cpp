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
#include "commodity.hpp"
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

int with_volatility( int what, int volatility ) {
  if( volatility >= 0 )
    return what * ( 1 << volatility );
  else
    return what * ( 1 >> ( -volatility ) );
}

// The sum of the player-traded volume across all players for a
// single commodity.
int total_traded_volume_for_commodity( SSConst const& ss,
                                       e_commodity    c ) {
  int sum = 0;
  for( auto const& [nation, player] : ss.players.players )
    if( player.has_value() )
      sum += player->old_world.market.commodities[c]
                 .player_traded_volume;
  return sum;
}

ProcessedGoodsPriceGroup create_price_group(
    SSConst const& ss, maybe<bool> is_dutch ) {
  ProcessedGoodsPriceGroupConfig config =
      default_processed_goods_price_group_config();
  if( is_dutch.has_value() and *is_dutch == true )
    config.dutch = true;
  for( e_processed_good good :
       refl::enum_values<e_processed_good> ) {
    e_commodity const comm = to_commodity( good );
    config.starting_intrinsic_volumes[good] =
        ss.players.global_market_state.commodities[comm]
            .intrinsic_volume;
    config.starting_traded_volumes[good] =
        total_traded_volume_for_commodity( ss, comm );
  }
  return ProcessedGoodsPriceGroup( config );
}

// Note that the intrinsic value passed in will generally already
// have a value that needs to be taken into account.
void try_price_change_default_model( Player const& player,
                                     e_commodity   type,
                                     int& intrinsic_volume_delta,
                                     int& price_change ) {
  PlayerMarketItem const player_market_item =
      player.old_world.market.commodities[type];
  auto const& item_config = config_market.price_behavior[type];
  int const   fall_threshold =
      item_config.model_parameters.fall * 100;
  int const rise_threshold =
      -item_config.model_parameters.rise * 100;
  int const new_player_volume =
      player_market_item.intrinsic_volume +
      intrinsic_volume_delta;
  if( new_player_volume >= fall_threshold ) {
    // Price drop.
    intrinsic_volume_delta -= fall_threshold;
    int const new_price =
        std::max( player_market_item.bid_price - 1,
                  item_config.price_limits.bid_price_min );
    price_change = new_price - player_market_item.bid_price;
  } else if( new_player_volume <= rise_threshold ) {
    // Price increase.
    intrinsic_volume_delta -= rise_threshold;
    int const new_price =
        std::min( player_market_item.bid_price + 1,
                  item_config.price_limits.bid_price_max );
    price_change = new_price - player_market_item.bid_price;
  }
  CHECK_GE( price_change, -1 );
  CHECK_LE( price_change, 1 );
}

PriceChange try_price_change_group_model(
    Player const& player, int equilibrium_price,
    e_commodity type, double volatility_push ) {
  int const ask_price = ask_from_bid(
      type,
      player.old_world.market.commodities[type].bid_price );
  // Need to clamp at both steps, since it is observed in the OG
  // that when the volatility_push becomes unity that it can
  // overtake the eq_push. And then we need to clamp the net_push
  // because in general in the OG the price can never move more
  // than one unit per sale.
  int const eq_push =
      clamp( equilibrium_price - ask_price, -1, 1 );
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
  int const price_change = net_push;
  CHECK_GE( price_change, -1 );
  CHECK_LE( price_change, 1 );
  int const current_bid = market_price( player, type ).bid;
  int const proposed_bid =
      clamp( current_bid + price_change,
             config_market.price_behavior[type]
                 .price_limits.bid_price_min,
             config_market.price_behavior[type]
                 .price_limits.bid_price_max );
  int const allowed_price_change = proposed_bid - current_bid;
  return create_price_change( player, type,
                              allowed_price_change );
}

Invoice transaction_invoice_default_model(
    SSConst const& ss, Player const& player,
    Commodity const orig_transacted,
    e_transaction   transaction_type,
    e_immediate_price_change_allowed
        immediate_price_change_allowed ) {
  // Make the quantity so that its sign reflects the sign of the
  // change in volume from the perspective of europe, since that
  // makes the below calculations easier.
  Commodity const transacted{
      .type     = orig_transacted.type,
      .quantity = ( transaction_type == e_transaction::buy )
                      ? -orig_transacted.quantity
                      : orig_transacted.quantity };

  CHECK( !is_in_processed_goods_price_group( transacted.type ) );
  CommodityPrice const prices =
      market_price( player, transacted.type );
  int const         quantity  = transacted.quantity;
  e_commodity const comm_type = transacted.type;
  auto const&       item_config =
      config_market.price_behavior[comm_type];
  int const volatility = item_config.model_parameters.volatility;
  auto const& difficulty_modifiers =
      config_market.difficulty_modifiers[ss.settings.difficulty];
  int const price = ( transaction_type == e_transaction::buy )
                        ? prices.ask
                        : prices.bid;

  Invoice invoice;
  invoice.what = orig_transacted;

  // 1. Player money adjustment.
  invoice.money_delta_before_taxes = price * transacted.quantity;
  invoice.tax_rate = player.old_world.taxes.tax_rate;
  if( transaction_type == e_transaction::sell ) {
    CHECK_GE( invoice.money_delta_before_taxes, 0 );
    // Rounding is not an issue here because the amount received
    // will always be a multiple of 100, since bid/ask prices in
    // the game are always so.
    invoice.tax_amount =
        int( invoice.tax_rate *
             ( invoice.money_delta_before_taxes / 100.0 ) );
  }
  invoice.money_delta_final =
      invoice.money_delta_before_taxes - invoice.tax_amount;

  // 2. Change player-traded volume (recall that this is defined
  // as the volume from the european perspective).
  invoice.player_volume_delta = quantity;

  // 3. Change the intrinsic volumes.
  double base_volume_change =
      with_volatility( quantity, volatility );
  base_volume_change *=
      ( ss.players.human == player.nation )
          ? difficulty_modifiers.human_traffic_volume_scale
          : difficulty_modifiers.non_human_traffic_volume_scale;

  for( e_nation nation : refl::enum_values<e_nation> ) {
    invoice.intrinsic_volume_delta[nation] = 0;
    maybe<Player const&> some_player =
        ss.players.players[nation];
    if( !some_player.has_value() ) continue;
    double const scale =
        ( transaction_type == e_transaction::sell )
            ? config_market.nation_advantage[nation]
                  .sell_volume_scale
            : config_market.nation_advantage[nation]
                  .buy_volume_scale;
    double const volume_change = base_volume_change * scale;
    // Here we want to round toward zero.
    invoice.intrinsic_volume_delta[nation] =
        int( volume_change );
  }

  // 4. Evolve the player's price and intrinsic volume for the
  // transacted commodity (only for player). Note that we need to
  // consider both rises and falls here, since the current in-
  // trinsic volume could have started off at any value.
  int price_change = 0;
  // The caller can suppress the immediate price change (and as-
  // sociated adjustment of intrinsic volume), which it will do
  // in the case of the custom house.
  if( immediate_price_change_allowed ==
      e_immediate_price_change_allowed::allowed )
    try_price_change_default_model(
        player, comm_type,
        invoice.intrinsic_volume_delta[player.nation],
        price_change );
  invoice.price_change =
      create_price_change( player, comm_type, price_change );

  CHECK_GE( invoice.price_change.to.bid,
            config_market.price_behavior[transacted.type]
                .price_limits.bid_price_min );
  CHECK_LE( invoice.price_change.to.bid,
            config_market.price_behavior[transacted.type]
                .price_limits.bid_price_max );
  return invoice;
}

Invoice transaction_invoice_processed_group_model(
    SSConst const& ss, Player const& player,
    Commodity transacted, e_transaction transaction_type,
    e_immediate_price_change_allowed
        immediate_price_change_allowed ) {
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
  bool const is_dutch = ( player.nation == e_nation::dutch );
  ProcessedGoodsPriceGroup group =
      create_price_group( ss, is_dutch );

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
  if( immediate_price_change_allowed ==
      e_immediate_price_change_allowed::allowed )
    invoice.price_change = try_price_change_group_model(
        player, equilibrium_prices[processed_type],
        transacted.type, volatility_push );
  else
    // The custom house needs to suppress immediate price
    // changes.
    invoice.price_change = create_price_change(
        player, transacted.type, /*price_change=*/0 );

  return invoice;
}

// This will evolve the european volumes for a single commodity,
// and for a single player, and it is done at the start of the
// that player's turn. In the default model it is the case that
// if the internal volume changes (which is what this function
// generally results in) then the price may need to be moved as
// well, and that in requires that the internal volume again be
// adjusted. Thus the two are coupled. Any price change that re-
// sults is returned.
PriceChange evolve_default_model_commodity(
    Player& player, e_commodity commodity ) {
  int intrinsic_volume_delta = 0;
  int price_change           = 0;

  // 1. Apply attrition. The attrition is applied before any po-
  // tential price changes are evaluated.
  intrinsic_volume_delta +=
      lround( config_market.price_behavior[commodity]
                  .model_parameters.attrition *
              config_market.nation_advantage[player.nation]
                  .attrition_scale );

  // 2. See if we should move the price. This will potentially
  // mutate the volume and price change variables.
  try_price_change_default_model(
      player, commodity, intrinsic_volume_delta, price_change );

  // Create this before affecting the changes.
  PriceChange const change =
      create_price_change( player, commodity, price_change );

  // Actually change the price.
  PlayerMarketItem& item =
      player.old_world.market.commodities[change.type];
  item.bid_price += price_change;
  item.intrinsic_volume += intrinsic_volume_delta;

  return change;
}

// This is done once at the start of each player turn.
refl::enum_map<e_commodity, PriceChange>
evolve_group_model_prices( SSConst const& ss, Player& player ) {
  refl::enum_map<e_commodity, PriceChange> res;

  // Note that there is no designated player in the context of
  // this evolution, so the is_dutch parameter is not relevant.
  // In the price group model the dutch status only comes into
  // play when buying/selling.
  ProcessedGoodsPriceGroup group =
      create_price_group( ss, /*is_dutch=*/nothing );
  auto equilibrium_prices = group.equilibrium_prices();

  // Attempt a price change and record it.
  for( e_processed_good good :
       refl::enum_values<e_processed_good> ) {
    e_commodity const comm   = to_commodity( good );
    PriceChange const change = try_price_change_group_model(
        player, equilibrium_prices[good], comm,
        /*volatility_push=*/0 );
    res[comm] = change;
    // Now make the change.
    player.old_world.market.commodities[comm].bid_price +=
        change.delta;
  }

  return res;
}

CommodityPrice make_commodity_price( e_commodity commodity,
                                     int         bid ) {
  int const ask = bid + config_market.price_behavior[commodity]
                            .price_limits.bid_ask_spread;
  return CommodityPrice{ .bid = bid, .ask = ask };
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
PriceChange create_price_change( Player const& player,
                                 e_commodity   comm,
                                 int           price_change ) {
  int const current_bid =
      player.old_world.market.commodities[comm].bid_price;
  int const current_ask = ask_from_bid( comm, current_bid );
  return PriceChange{
      .type  = comm,
      .from  = CommodityPrice{ .bid = current_bid,
                               .ask = current_ask },
      .to    = CommodityPrice{ .bid = current_bid + price_change,
                               .ask = current_ask + price_change },
      .delta = price_change };
}

CommodityPrice market_price( Player const& player,
                             e_commodity   commodity ) {
  int const bid =
      player.old_world.market.commodities[commodity].bid_price;
  return make_commodity_price( commodity, bid );
}

int ask_from_bid( e_commodity type, int bid ) {
  return bid + config_market.price_behavior[type]
                   .price_limits.bid_ask_spread;
}

Invoice transaction_invoice(
    SSConst const& ss, Player const& player,
    Commodity transacted, e_transaction transaction_type,
    e_immediate_price_change_allowed
        immediate_price_change_allowed ) {
  Invoice invoice;
  if( is_in_processed_goods_price_group( transacted.type ) )
    invoice = transaction_invoice_processed_group_model(
        ss, player, transacted, transaction_type,
        immediate_price_change_allowed );
  else
    invoice = transaction_invoice_default_model(
        ss, player, transacted, transaction_type,
        immediate_price_change_allowed );
  CHECK_GE( invoice.price_change.to.bid,
            config_market.price_behavior[transacted.type]
                .price_limits.bid_price_min );
  CHECK_LE( invoice.price_change.to.bid,
            config_market.price_behavior[transacted.type]
                .price_limits.bid_price_max );
  return invoice;
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
      .bid_price += invoice.price_change.delta;
}

wait<> display_price_change_notification(
    TS& ts, Player const& player, PriceChange const& change ) {
  if( change.from == change.to ) co_return;
  string const harbor_name =
      nation_obj( player.nation ).harbor_city_name;
  CHECK( change.to.bid - change.from.bid ==
         change.to.ask - change.from.ask );
  int const    price_change = change.to.bid - change.from.bid;
  string const verb = ( price_change > 0 ) ? "risen" : "fallen";
  CommodityPrice const prices = change.to;
  string const         msg    = fmt::format(
      "The price of [{}] in {} has {} to {}.",
      lowercase_commodity_display_name( change.type ),
      harbor_name, verb, prices.bid );
  co_await ts.gui.message_box( msg );
}

bool is_in_processed_goods_price_group( e_commodity type ) {
  switch( type ) {
    case e_commodity::rum:
    case e_commodity::cigars:
    case e_commodity::cloth:
    case e_commodity::coats: //
      return true;
    default:                 //
      return false;
  }
}

void evolve_group_model_volumes( SS& ss ) {
  // Note that there is no designated player in the context of
  // this evolution, so the is_dutch parameter is not relevant.
  // In the price group model the dutch status only comes into
  // play when buying/selling.
  ProcessedGoodsPriceGroup group =
      create_price_group( ss, /*is_dutch=*/nothing );

  // Do the evolution (only affects intrinsic volumes).
  group.evolve();

  for( e_processed_good good :
       refl::enum_values<e_processed_good> ) {
    e_commodity const comm = to_commodity( good );
    ss.players.global_market_state.commodities[comm]
        .intrinsic_volume = group.intrinsic_volume( good );
  }
}

refl::enum_map<e_commodity, PriceChange> evolve_player_prices(
    SSConst const& ss, Player& player ) {
  refl::enum_map<e_commodity, PriceChange> res;
  refl::enum_map<e_commodity, PriceChange> const
      processed_goods = evolve_group_model_prices( ss, player );
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    if( is_in_processed_goods_price_group( c ) )
      res[c] = processed_goods[c];
    else
      res[c] = evolve_default_model_commodity( player, c );
    CHECK_GE( res[c].to.bid, config_market.price_behavior[c]
                                 .price_limits.bid_price_min );
    CHECK_LE( res[c].to.bid, config_market.price_behavior[c]
                                 .price_limits.bid_price_max );
  }
  return res;
}

PriceLimits price_limits_for_commodity( e_commodity comm ) {
  auto& conf = config_market.price_behavior[comm];
  return { .low = make_commodity_price(
               comm, conf.price_limits.bid_price_min ),
           .high = make_commodity_price(
               comm, conf.price_limits.bid_price_max ) };
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

LUA_FN( bid_ask_spread, int, e_commodity comm ) {
  return config_market.price_behavior[comm]
      .price_limits.bid_ask_spread;
}

} // namespace
} // namespace rn
