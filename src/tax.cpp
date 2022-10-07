/****************************************************************
**tax.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Handles things related to tax increases and
*              boycotts.
*
*****************************************************************/
#include "tax.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "commodity.hpp"
#include "igui.hpp"
#include "irand.hpp"
#include "market.hpp"
#include "ts.hpp"

// config
#include "config/commodity.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/old-world.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.hpp"

using namespace std;

namespace rn {

namespace {

int rand_int_range( TS& ts, auto const& int_range ) {
  int const min = int_range.min;
  int const max = int_range.max;
  return ts.rand.between_ints( min, max,
                               IRand::e_interval::closed );
}

double rand_dbl_range( TS& ts, auto const& dbl_range ) {
  double const min = dbl_range.min;
  double const max = dbl_range.max;
  return ts.rand.between_doubles( min, max );
}

string ordinal_for( int n ) {
  n = n % 100;
  if( n >= 20 ) n = n % 10;
  switch( n ) {
    case 1: return "st";
    case 2: return "nd";
    case 3: return "rd";
    default: return "th";
  }
}

// This will find the commodity cargo that has the highest value
// (in the sense of bid price) out of all the player's colonies
// that isn't already boycotted.
maybe<CommodityInColony> find_what_to_boycott(
    SSConst const& ss, Player const& player ) {
  refl::enum_map<e_commodity, /*bid=*/int> prices;
  for( auto& [comm, bid] : prices )
    bid = market_price( player, comm ).bid;
  vector<ColonyId> const player_colonies =
      ss.colonies.for_nation( player.nation );
  maybe<CommodityInColony> res;
  int                      largest_value = 0;
  for( ColonyId const colony_id : player_colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    for( auto& [comm, uncapped_quantity] : colony.commodities ) {
      if( player.old_world.market.commodities[comm].boycott )
        continue;
      int const quantity =
          std::min( uncapped_quantity,
                    config_old_world.boycotts
                        .max_commodity_quantity_to_throw );
      if( quantity == 0 ) continue;
      int const value = prices[comm] * quantity;
      if( value < largest_value ) continue;
      if( !res.has_value() ) res.emplace();
      largest_value = value;

      res = CommodityInColony{
          .colony_id         = colony_id,
          .type_and_quantity = { .type     = comm,
                                 .quantity = quantity } };
    }
  }
  return res;
}

wait<> boycott_msg( SSConst const& ss, TS& ts,
                    Player const&                 player,
                    TaxChangeResult::party const& party ) {
  string_view const colony_name =
      ss.colonies.colony_for( party.how.commodity.colony_id )
          .name;
  string const lower_commodity_name =
      lowercase_commodity_display_name(
          party.how.commodity.type_and_quantity.type );
  string const upper_commodity_name =
      uppercase_commodity_display_name(
          party.how.commodity.type_and_quantity.type );
  string_view const country_adjective =
      config_nation.nations[player.nation].adjective;
  string_view const harbor_city_name =
      config_nation.nations[player.nation].harbor_city_name;
  int const quantity =
      party.how.commodity.type_and_quantity.quantity;

  string const msg = fmt::format(
      "Colonists in {} hold @[H]{} Party@[]!  Amid colonists "
      "refusal to pay new tax, Sons of Liberty throw @[H]{}@[] "
      "tons of {} into the sea!  The {} Parliament announces "
      "boycott of {}.  {} cannot be traded in {} until boycott "
      "is lifted.",
      colony_name, upper_commodity_name, quantity,
      lower_commodity_name, country_adjective,
      lower_commodity_name, upper_commodity_name,
      harbor_city_name );
  co_await ts.gui.message_box( msg );
}

int remarry( TS& ts, Player& player ) {
  int const curr_wife =
      player.old_world.taxes.king_remarriage_count +
      config_old_world.min_king_wife_number;
  int const remarriages_since_last_tax_event =
      ts.rand.between_ints( 1, 3, IRand::e_interval::closed );
  int const new_wife =
      curr_wife + remarriages_since_last_tax_event;
  player.old_world.taxes.king_remarriage_count +=
      remarriages_since_last_tax_event;
  return new_wife;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
TaxUpdateComputation compute_tax_change( SSConst const& ss,
                                         TS&            ts,
                                         Player const& player ) {
  TaxationState const& state = player.old_world.taxes;
  int const            turn  = ss.turn.time_point.turns;
  TaxUpdateComputation update;
  update.next_tax_event_turn = state.next_tax_event_turn;
  if( state.next_tax_event_turn > turn )
    // Most common case, we have nothing to do.
    return update;
  auto& tax_config =
      config_old_world.taxes[ss.settings.difficulty];
  int const next =
      turn +
      rand_int_range(
          ts, tax_config.turns_between_tax_change_events );
  // The next tax event turn is at or behind us, and so whatever
  // happens we need to advance it.
  update.next_tax_event_turn = next;
  if( state.next_tax_event_turn == 0 )
    // No tax changes on the first turn.
    return update;
  // Check for remaining blockers, year and #colonies.
  if( turn < tax_config.min_turn_for_tax_change_events )
    return update;
  if( ss.colonies.for_nation( player.nation ).size() == 0 )
    // This is basically to prevent tax increases too early in
    // the game, although if the player finds themselves with no
    // colonies later in the game, the OG also pauses tax in-
    // creases.
    return update;
  // We have a tax event this turn. Whatever happens, we will
  // need an amount.
  int const amount =
      rand_int_range( ts, tax_config.tax_rate_change );
  // Next determine whether it's an increase or decrease, since
  // if it's the latter then our job is easy.
  bool const increase =
      ts.rand.bernoulli( tax_config.tax_increase_probability );
  int const curr_tax = player.old_world.taxes.tax_rate;
  if( !increase ) {
    // Make sure the tax rate doesn't go below zero.
    int const new_tax_rate   = std::max( curr_tax - amount, 0 );
    int const clamped_amount = curr_tax - new_tax_rate;
    CHECK_GE( clamped_amount, 0 );
    update.proposed_tax_change =
        TaxChangeProposal::decrease{ .amount = clamped_amount };
    return update;
  }
  // We have an increase.
  if( curr_tax == tax_config.maximum_tax_rate ) return update;
  int const new_tax_rate =
      std::min( curr_tax + amount, tax_config.maximum_tax_rate );
  int const clamped_amount = new_tax_rate - curr_tax;
  CHECK_GE( clamped_amount, 0 );
  maybe<CommodityInColony> const boycott =
      find_what_to_boycott( ss, player );
  if( !boycott.has_value() ) {
    // It's pretty rare that this happens: a tax increase is hap-
    // pening but there are either no colonies or there are zero
    // commodities in all colonies, so there is nothing to boy-
    // cott, therefore the tax increase must be accepted by the
    // player.
    update.proposed_tax_change =
        TaxChangeProposal::increase{ .amount = clamped_amount };
    return update;
  }
  CHECK( boycott.has_value() );
  double const rebels_bump = rand_dbl_range(
      ts, config_old_world.boycotts.rebels_bump_after_party );
  TeaParty const party = { .commodity   = *boycott,
                           .rebels_bump = rebels_bump };
  update.proposed_tax_change =
      TaxChangeProposal::increase_or_party{
          .amount = clamped_amount, .party = party };
  return update;
}

wait<TaxChangeResult_t> prompt_for_tax_change_result(
    SSConst const& ss, TS& ts, Player& player,
    TaxChangeProposal_t const& change ) {
  string_view constexpr increase_msg =
      "In honor of our marriage to our {}{} wife, we have "
      "decided to raise your tax rate by @[H]{}%@[].  The tax "
      "rate is now @[H]{}%@[].";
  string const decrease_msg =
      "The crown has graciously decided to LOWER your tax rate "
      "by @[H]{}%@[].  The tax rate is now @[H]{}%@[].";
  switch( change.to_enum() ) {
    case TaxChangeProposal::e::none:
      co_return TaxChangeResult::none{};
    case TaxChangeProposal::e::increase: {
      auto&     o = change.get<TaxChangeProposal::increase>();
      int const new_wife = remarry( ts, player );
      co_await ts.gui.message_box(
          increase_msg, new_wife, ordinal_for( new_wife ),
          o.amount, player.old_world.taxes.tax_rate + o.amount );
      co_return TaxChangeResult::tax_change{ .amount =
                                                 o.amount };
    }
    case TaxChangeProposal::e::increase_or_party: {
      auto& o =
          change.get<TaxChangeProposal::increase_or_party>();
      static string const kiss =
          " We will graciously allow you to kiss our royal "
          "pinky ring.";
      int const    new_wife = remarry( ts, player );
      string const msg =
          fmt::format(
              increase_msg, new_wife, ordinal_for( new_wife ),
              o.amount,
              player.old_world.taxes.tax_rate + o.amount ) +
          kiss;
      string const party = fmt::format(
          "Hold '@[H]{} {} party@[]'!",
          ss.colonies.colony_for( o.party.commodity.colony_id )
              .name,
          uppercase_commodity_display_name(
              o.party.commodity.type_and_quantity.type ) );
      YesNoConfig const config{
          .msg            = msg,
          .yes_label      = "Kiss pinky ring.",
          .no_label       = party,
          .no_comes_first = false,
      };
      ui::e_confirm const answer =
          co_await ts.gui.required_yes_no( config );
      switch( answer ) {
        case ui::e_confirm::yes:
          co_return TaxChangeResult::tax_change{ .amount =
                                                     o.amount };
        case ui::e_confirm::no:
          co_return TaxChangeResult::party{ .how = o.party };
      }
    }
    case TaxChangeProposal::e::decrease: {
      auto& o = change.get<TaxChangeProposal::decrease>();
      co_await ts.gui.message_box(
          decrease_msg, o.amount,
          player.old_world.taxes.tax_rate + o.amount );
      co_return TaxChangeResult::tax_change{ .amount =
                                                 -o.amount };
    }
  }
}

void apply_tax_result( SS& ss, Player& player,
                       int next_tax_event_turn,
                       TaxChangeResult_t const& change ) {
  CHECK_GT( next_tax_event_turn, ss.turn.time_point.turns );
  player.old_world.taxes.next_tax_event_turn =
      next_tax_event_turn;
  switch( change.to_enum() ) {
    case TaxChangeResult::e::none: return;
    case TaxChangeResult::e::tax_change: {
      auto& o = change.get<TaxChangeResult::tax_change>();
      player.old_world.taxes.tax_rate += o.amount;
      CHECK_GE( player.old_world.taxes.tax_rate, 0 );
      CHECK_LE( player.old_world.taxes.tax_rate,
                config_old_world.taxes[ss.settings.difficulty]
                    .maximum_tax_rate );
      return;
    }
    case TaxChangeResult::e::party: {
      auto&   o = change.get<TaxChangeResult::party>();
      Colony& colony =
          ss.colonies.colony_for( o.how.commodity.colony_id );
      Commodity const& to_throw =
          o.how.commodity.type_and_quantity;
      CHECK_GE( o.how.rebels_bump, 0 );
      colony.sons_of_liberty.num_rebels_from_bells_only +=
          o.how.rebels_bump;
      int& quantity = colony.commodities[to_throw.type];
      quantity -= to_throw.quantity;
      // Boycott the commodity.
      CHECK( !player.old_world.market.commodities[to_throw.type]
                  .boycott );
      player.old_world.market.commodities[to_throw.type]
          .boycott = true;
      CHECK_GE( quantity, 0 );
      return;
    }
  }
}

wait<> start_of_turn_tax_check( SS& ss, TS& ts,
                                Player& player ) {
  TaxUpdateComputation const update =
      compute_tax_change( ss, ts, player );
  CHECK_GT( update.next_tax_event_turn,
            ss.turn.time_point.turns );
  // We have a tax increase or decrease.
  TaxChangeResult_t const result =
      co_await prompt_for_tax_change_result(
          ss, ts, player, update.proposed_tax_change );
  apply_tax_result( ss, player, update.next_tax_event_turn,
                    result );
  if( auto party = result.get_if<TaxChangeResult::party>();
      party.has_value() )
    co_await boycott_msg( ss, ts, player, *party );
}

int back_tax_for_boycotted_commodity( Player const& player,
                                      e_commodity   type ) {
  return market_price( player, type ).ask *
         config_old_world.boycotts.back_taxes_ask_multiplier;
}

wait<> try_trade_boycotted_commodity( TS& ts, Player& player,
                                      e_commodity type,
                                      int         back_taxes ) {
  bool& boycott =
      player.old_world.market.commodities[type].boycott;
  CHECK( boycott );
  string_view const to_be =
      config_commodity.types[type].plural ? "are" : "is";
  string const msg = fmt::format(
      "@[H]{}@[] {} current under Parliamentary boycott. We "
      "cannot trade {} until Parliament lifts the boycott, "
      "which it will not do unless we agree to pay @[H]{}@[] in "
      "back taxes.",
      uppercase_commodity_display_name( type ), to_be,
      lowercase_commodity_display_name( type ), back_taxes );
  if( player.money < back_taxes ) {
    // Player can't afford it.
    co_await ts.gui.message_box(
        msg + fmt::format( " Treasury: {}.", player.money ) );
    co_return;
  }
  YesNoConfig const config{
      .msg       = msg,
      .yes_label = fmt::format( "Pay @[H]{}@[].", back_taxes ),
      .no_label  = "This is taxation without representation!",
      .no_comes_first = true,
  };
  ui::e_confirm const answer =
      co_await ts.gui.required_yes_no( config );
  switch( answer ) {
    case ui::e_confirm::yes:
      // Lift the boycott.
      boycott = false;
      player.money -= back_taxes;
      CHECK_GE( player.money, 0 );
      co_return;
    case ui::e_confirm::no: co_return;
  }
}

} // namespace rn
