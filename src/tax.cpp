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
#include "connectivity.hpp"
#include "iagent.hpp"
#include "igui.hpp"
#include "irand.hpp"
#include "isignal.rds.hpp"
#include "market.hpp"
#include "player-mgr.hpp"

// config
#include "config/commodity.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/old-world.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::NoDiscard;

int rand_int_range( IRand& rand, auto const& int_range ) {
  int const min = int_range.min;
  int const max = int_range.max;
  return rand.uniform_int( min, max );
}

int rand_int_range_by_year(
    int const year, IRand& rand,
    config::old_world::TaxChangeRanges const& ranges ) {
  config::IntRange const& range =
      ( year < 1600 )   ? ranges.years_0000_to_1599
      : ( year < 1700 ) ? ranges.years_1600_to_1699
      : ( year < 1800 ) ? ranges.years_1700_to_1799
                        : ranges.years_1800_to_9999;
  return rand.uniform_int( range.min, range.max );
}

double rand_dbl_range( IRand& rand, auto const& dbl_range ) {
  double const min = dbl_range.min;
  double const max = dbl_range.max;
  return rand.uniform_double( min, max );
}

string ordinal_for( int n ) {
  n = n % 100;
  if( n >= 20 ) n = n % 10;
  switch( n ) {
    case 1:
      return "st";
    case 2:
      return "nd";
    case 3:
      return "rd";
    default:
      return "th";
  }
}

// This will find the commodity cargo that has the highest value
// (in the sense of bid price) out of all the player's colonies
// that isn't already boycotted, but only from colonies that have
// ocean access.
maybe<CommodityInColony> find_what_to_boycott(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    Player const& player ) {
  refl::enum_map<e_commodity, /*bid=*/int> prices;
  for( auto& [comm, bid] : prices )
    bid = market_price( ss, player, comm ).bid;
  vector<ColonyId> const player_colonies =
      ss.colonies.for_player( player.type );
  maybe<CommodityInColony> res;
  int largest_value = 0;
  for( ColonyId const colony_id : player_colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    if( !tile_has_surrounding_ocean_access( ss, connectivity,
                                            colony.location ) )
      // In the OG, only colonies that have ocean (sea lane) ac-
      // cess can boycott, because boycotting entails "throwing
      // goods into the sea."
      continue;
    for( auto& [comm, uncapped_quantity] : colony.commodities ) {
      if( old_world_state( ss, player.type )
              .market.commodities[comm]
              .boycott )
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

wait<> boycott_msg( SSConst const& ss, Player const& player,
                    IAgent& agent,
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
  // Here we don't need to use player_possessive since boycotting
  // won't ever be done after declaration.
  string_view const country_possessive =
      config_nation.players[player.type]
          .possessive_pre_declaration;
  string_view const harbor_city_name =
      config_nation.nations[player.nation].harbor_city_name;
  int const quantity =
      party.how.commodity.type_and_quantity.quantity;

  string const msg = fmt::format(
      "Colonists in {} hold [{} Party]!  Amid colonists' "
      "refusal to pay new tax, Sons of Liberty throw [{}] "
      "tons of {} into the sea!  The {} Parliament announces "
      "boycott of {}.  {} cannot be traded in {} until boycott "
      "is lifted.",
      colony_name, upper_commodity_name, quantity,
      lower_commodity_name, country_possessive,
      lower_commodity_name, upper_commodity_name,
      harbor_city_name );
  co_await agent.signal(
      signal::TeaParty{
        .what      = party.how.commodity.type_and_quantity,
        .colony_id = party.how.commodity.colony_id },
      msg );
}

int remarry( SS& ss, IRand& rand, Player& player ) {
  int const curr_wife = old_world_state( ss, player.type )
                            .taxes.king_remarriage_count +
                        config_old_world.min_king_wife_number;
  int const remarriages_since_last_tax_event =
      rand.uniform_int( 1, 3 );
  int const new_wife =
      curr_wife + remarriages_since_last_tax_event;
  old_world_state( ss, player.type )
      .taxes.king_remarriage_count +=
      remarriages_since_last_tax_event;
  return new_wife;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
TaxUpdateComputation compute_tax_change(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    IRand& rand, Player const& player ) {
  int const year = ss.turn.time_point.year;
  auto const& tax_config =
      config_old_world
          .taxes[ss.settings.game_setup_options.difficulty];
  TaxUpdateComputation update;
  update.next_tax_event_turn = [&] {
    TaxationState const& state =
        old_world_state( ss, player.type ).taxes;
    if( state.next_tax_event_turn == 0 )
      // As in the OG, set the first tax event turn to be the
      // fixed value specified in the config.
      return tax_config.min_turn_for_tax_change_events;
    return state.next_tax_event_turn;
  }();
  int const turn = ss.turn.time_point.turns;
  if( turn < update.next_tax_event_turn )
    // Most common case, we have nothing to do.
    return update;
  // The next tax event turn is at or behind us, and so whatever
  // happens we need to advance it.
  update.next_tax_event_turn =
      turn + rand_int_range_by_year(
                 year, rand,
                 tax_config.turns_between_tax_change_events );
  if( ss.colonies.for_player( player.type ).size() == 0 )
    // This is basically to prevent tax increases too early in
    // the game, although if the player finds themselves with no
    // colonies later in the game, the OG also pauses tax in-
    // creases.
    return update;
  int const curr_tax =
      old_world_state( ss, player.type ).taxes.tax_rate;
  if( curr_tax > tax_config.maximum_tax_rate )
    // This will happen when the player uses cheat mode to fiddle
    // with the tax rate, specifically making it higher than the
    // maximum that is normally possible on the current diffi-
    // culty level. Or they could have used modding or cheat mode
    // to lower the difficulty level mid-game. We should be de-
    // fensive here and not do anything (even lowering the tax
    // rate) because if we do then we risk check-failing later in
    // the subsequent logic that ensures that the tax rate
    // doesn't end up outside the allowed range from the config.
    return update;
  // We have a tax event this turn. Whatever happens, we will
  // need an amount.
  int const amount =
      rand_int_range( rand, tax_config.tax_rate_change );
  // Next determine whether it's an increase or decrease, since
  // if it's the latter then our job is easy.
  bool const increase =
      rand.bernoulli( tax_config.tax_increase.probability );
  if( !increase ) {
    if( curr_tax == 0 ) return update;
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
      find_what_to_boycott( ss, connectivity, player );
  if( !boycott.has_value() ) {
    // It's pretty rare that this happens: a tax increase is hap-
    // pening but there are either no colonies or there are no
    // colonies with commodities in them that also have ocean ac-
    // cess. So there is nothing to boycott, therefore the tax
    // increase must be accepted by the player.
    update.proposed_tax_change =
        TaxChangeProposal::increase{ .amount = clamped_amount };
    return update;
  }
  CHECK( boycott.has_value() );
  double const rebels_bump = rand_dbl_range(
      rand, config_old_world.boycotts.rebels_bump_after_party );
  TeaParty const party = { .commodity   = *boycott,
                           .rebels_bump = rebels_bump };
  update.proposed_tax_change =
      TaxChangeProposal::increase_or_party{
        .amount = clamped_amount, .party = party };
  return update;
}

wait<TaxChangeResult> prompt_for_tax_change_result(
    SS& ss, IRand& rand, Player& player_non_const, IAgent& agent,
    TaxChangeProposal const& proposal ) {
  Player const& player = player_non_const;
  static string_view const constexpr increase_msg =
      "In honor of our marriage to our {}{} wife, we have "
      "decided to raise your tax rate by [{}%].  The tax "
      "rate is now [{}%]. We will graciously allow you to "
      "kiss our royal pinky ring.";
  switch( proposal.to_enum() ) {
    case TaxChangeProposal::e::none:
      co_return TaxChangeResult::none{};
    case TaxChangeProposal::e::increase: {
      auto& o = proposal.get<TaxChangeProposal::increase>();
      int const new_wife = remarry( ss, rand, player_non_const );
      string const msg   = fmt::format(
          increase_msg, new_wife, ordinal_for( new_wife ),
          o.amount,
          old_world_state( ss, player.type ).taxes.tax_rate +
              o.amount );
      co_await agent.signal(
          signal::TaxRateWillChange{ .delta = o.amount }, msg );
      co_return TaxChangeResult::tax_change{ .amount =
                                                 o.amount };
    }
    case TaxChangeProposal::e::increase_or_party: {
      auto& o =
          proposal.get<TaxChangeProposal::increase_or_party>();
      int const new_wife = remarry( ss, rand, player_non_const );
      string const msg   = fmt::format(
          increase_msg, new_wife, ordinal_for( new_wife ),
          o.amount,
          old_world_state( ss, player.type ).taxes.tax_rate +
              o.amount );
      ui::e_confirm const answer =
          co_await agent.kiss_pinky_ring(
              msg, o.party.commodity.colony_id,
              o.party.commodity.type_and_quantity.type,
              o.amount );
      switch( answer ) {
        case ui::e_confirm::yes:
          agent.signal(
              signal::TaxRateWillChange{ .delta = o.amount } );
          co_return TaxChangeResult::tax_change{ .amount =
                                                     o.amount };
        case ui::e_confirm::no:
          // A signal is sent for this later.
          co_return TaxChangeResult::party{ .how = o.party };
      }
      FATAL( "unexpected value for e_confirm enum: {}",
             static_cast<int>( answer ) );
    }
    case TaxChangeProposal::e::decrease: {
      auto& o = proposal.get<TaxChangeProposal::decrease>();
      string_view constexpr decrease_msg =
          "The crown has graciously decided to LOWER your tax "
          "rate by [{}%].  The tax rate is now [{}%].";
      string const msg = fmt::format(
          decrease_msg, o.amount,
          old_world_state( ss, player.type ).taxes.tax_rate -
              o.amount );
      co_await agent.signal(
          signal::TaxRateWillChange{ .delta = -o.amount }, msg );
      co_return TaxChangeResult::tax_change{ .amount =
                                                 -o.amount };
    }
  }
}

void apply_tax_result( SS& ss, Player& player,
                       int next_tax_event_turn,
                       TaxChangeResult const& change ) {
  CHECK_GT( next_tax_event_turn, ss.turn.time_point.turns );
  old_world_state( ss, player.type ).taxes.next_tax_event_turn =
      next_tax_event_turn;
  switch( change.to_enum() ) {
    case TaxChangeResult::e::none:
      return;
    case TaxChangeResult::e::tax_change: {
      auto& o = change.get<TaxChangeResult::tax_change>();
      old_world_state( ss, player.type ).taxes.tax_rate +=
          o.amount;
      CHECK_GE(
          old_world_state( ss, player.type ).taxes.tax_rate, 0 );
      CHECK_LE(
          old_world_state( ss, player.type ).taxes.tax_rate,
          config_old_world
              .taxes[ss.settings.game_setup_options.difficulty]
              .maximum_tax_rate );
      return;
    }
    case TaxChangeResult::e::party: {
      auto& o = change.get<TaxChangeResult::party>();
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
      CHECK( !old_world_state( ss, player.type )
                  .market.commodities[to_throw.type]
                  .boycott );
      old_world_state( ss, player.type )
          .market.commodities[to_throw.type]
          .boycott = true;
      CHECK_GE( quantity, 0 );
      return;
    }
  }
}

wait<> start_of_turn_tax_check(
    SS& ss, IRand& rand, TerrainConnectivity const& connectivity,
    Player& player, IAgent& agent ) {
  TaxUpdateComputation const update =
      compute_tax_change( ss, connectivity, rand, player );
  CHECK_GT( update.next_tax_event_turn,
            ss.turn.time_point.turns );
  // We have a tax increase or decrease.
  TaxChangeResult const result =
      co_await prompt_for_tax_change_result(
          ss, rand, player, agent, update.proposed_tax_change );
  apply_tax_result( ss, player, update.next_tax_event_turn,
                    result );
  if( auto party = result.get_if<TaxChangeResult::party>();
      party.has_value() )
    co_await boycott_msg( ss, player, agent, *party );
}

int back_tax_for_boycotted_commodity( SSConst const& ss,
                                      Player const& player,
                                      e_commodity type ) {
  return market_price( ss, player, type ).ask *
         config_old_world.boycotts.back_taxes_ask_multiplier;
}

// This one is only called by human players in the harbor view,
// so can use the IGui interface.
wait_bool try_trade_boycotted_commodity( SS& ss, IGui& gui,
                                         Player& player,
                                         e_commodity type,
                                         int back_taxes ) {
  bool& boycott = old_world_state( ss, player.type )
                      .market.commodities[type]
                      .boycott;
  CHECK( boycott );
  string_view const to_be =
      config_commodity.types[type].plural ? "are" : "is";
  string const msg = fmt::format(
      "[{}] {} currently under Parliamentary boycott. We "
      "cannot trade {} until Parliament lifts the boycott, "
      "which it will not do unless we agree to pay [{}] in "
      "back taxes.",
      uppercase_commodity_display_name( type ), to_be,
      lowercase_commodity_display_name( type ), back_taxes );
  if( player.money < back_taxes ) {
    // Player can't afford it.
    co_await gui.message_box(
        msg + fmt::format( " Treasury: {}.", player.money ) );
    co_return boycott;
  }
  YesNoConfig const config{
    .msg            = msg,
    .yes_label      = fmt::format( "Pay [{}].", back_taxes ),
    .no_label       = "This is taxation without representation!",
    .no_comes_first = true,
  };
  auto const answer = co_await gui.optional_yes_no( config );
  switch( answer.value_or( ui::e_confirm::no ) ) {
    case ui::e_confirm::yes:
      // Lift the boycott.
      boycott = false;
      player.money -= back_taxes;
      CHECK_GE( player.money, 0 );
      break;
    case ui::e_confirm::no:
      break;
  }
  co_return boycott;
}

} // namespace rn
