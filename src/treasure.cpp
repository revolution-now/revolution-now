/****************************************************************
**treasure.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-17.
*
* Description: Things related to treasure trains.
*
*****************************************************************/
#include "treasure.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "irand.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/old-world.rds.hpp"
#include "config/text.rds.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/revolution.rds.hpp"
#include "ss/settings.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// base
#include "base/math.hpp"

using namespace std;

namespace rn {

namespace {

int king_transport_cost_percent( int tax_rate,
                                 e_difficulty difficulty ) {
  auto const& config = config_old_world.treasure[difficulty];
  return clamp( tax_rate * config.king_transport_tax_multiplier,
                config.king_transport_cut_range.min,
                config.king_transport_cut_range.max );
}

bool player_has_galleons( SSConst const& ss,
                          Player const& player ) {
  unordered_map<UnitId, UnitState::euro const*> const&
      units_all = ss.units.euro_all();
  for( auto& [unit_id, state] : units_all ) {
    Unit const& unit = ss.units.unit_for( unit_id );
    if( unit.player_type() != player.type ) continue;
    if( unit.type() == e_unit_type::galleon ) return true;
  }
  return false;
}

TreasureReceipt treasure_king_transport_receipt(
    SSConst const& ss, Player const& player,
    Unit const& treasure ) {
  bool const no_extra_charge =
      player.fathers.has[e_founding_father::hernan_cortes];
  e_treasure_transport_mode const transport_mode =
      no_extra_charge
          ? e_treasure_transport_mode::king_no_extra_charge
          : e_treasure_transport_mode::king_with_charge;
  int const worth =
      treasure.composition().inventory()[e_unit_inventory::gold];
  int const tax_rate = player.old_world.taxes.tax_rate;
  int const cut_percent =
      no_extra_charge
          ? tax_rate
          : king_transport_cost_percent(
                tax_rate,
                ss.settings.game_setup_options.difficulty );
  int const cut =
      static_cast<int>( worth * ( cut_percent / 100.0 ) );
  int const net = std::max( worth - cut, 0 );
  return TreasureReceipt{ .treasure_id       = treasure.id(),
                          .transport_mode    = transport_mode,
                          .original_worth    = worth,
                          .kings_cut_percent = cut_percent,
                          .net_received      = net };
}

} // namespace

TreasureReceipt treasure_in_harbor_receipt(
    Player const& player, Unit const& treasure ) {
  e_treasure_transport_mode const transport_mode =
      e_treasure_transport_mode::player;
  int const worth =
      treasure.composition().inventory()[e_unit_inventory::gold];
  int const tax_rate    = player.old_world.taxes.tax_rate;
  int const cut_percent = tax_rate;
  int const cut =
      static_cast<int>( worth * ( cut_percent / 100.0 ) );
  // Defensive, if tax rate is over 100.
  int const net = std::max( worth - cut, 0 );
  return TreasureReceipt{ .treasure_id       = treasure.id(),
                          .transport_mode    = transport_mode,
                          .original_worth    = worth,
                          .kings_cut_percent = cut_percent,
                          .net_received      = net };
}

wait<maybe<TreasureReceipt>> treasure_enter_colony(
    SSConst const& ss, TS& ts, Player const& player,
    Unit const& treasure ) {
  if( player.revolution.status >=
      e_revolution_status::declared ) {
    // After independence is declared, a message just pops up
    // saying "Treasure sold to traveling merchants for XXX",
    // where XXX is the full value of the treasure since there
    // are no more taxes. This is necessary because the player
    // can no longer transport it to europe.
    co_return TreasureReceipt{
      .treasure_id = treasure.id(),
      .transport_mode =
          e_treasure_transport_mode::traveling_merchants,
      .original_worth = treasure.composition()
                            .inventory()[e_unit_inventory::gold],
      .kings_cut_percent = 0,
      .net_received      = treasure.composition()
                          .inventory()[e_unit_inventory::gold],
    };
  }
  TreasureReceipt const receipt =
      treasure_king_transport_receipt( ss, player, treasure );
  string msg =
      "The crown is happy to see the bounty that you've "
      "acquired. ";
  if( !player_has_galleons( ss, player ) )
    msg +=
        "Seeing that you don't have a fleet of Galleons with "
        "which ";
  else
    msg += "Because we don't want you to be burdened by having ";
  msg += fmt::format(
      "to transport this treasure to {} we will happily "
      "transport it for you, ",
      config_nation.nations[player.nation].harbor_city_name );
  switch( receipt.transport_mode ) {
    case e_treasure_transport_mode::player:
      SHOULD_NOT_BE_HERE;
    case e_treasure_transport_mode::traveling_merchants:
      SHOULD_NOT_BE_HERE;
    case e_treasure_transport_mode::king_with_charge:
      msg +=
          "after which we will make an assessment of the "
          "appropriate percentage to withhold as compensation.";
      break;
    case e_treasure_transport_mode::king_no_extra_charge:
      msg +=
          "and we will do so for [no extra charge], only "
          "withholding an amount determined by the current tax "
          "rate.";
      break;
  }
  YesNoConfig const config{
    .msg            = msg,
    .yes_label      = "Accept.",
    .no_label       = "Decline.",
    .no_comes_first = false,
  };
  maybe<ui::e_confirm> const choice =
      co_await ts.gui.optional_yes_no( config );
  if( choice != ui::e_confirm::yes ) co_return nothing;
  co_return receipt;
}

void apply_treasure_reimbursement(
    SS& ss, Player& player, TreasureReceipt const& receipt ) {
  UnitOwnershipChanger( ss, receipt.treasure_id ).destroy();
  player.money += receipt.net_received;
  player.total_after_tax_revenue += receipt.net_received;
  int const king_tax_revenue_received =
      receipt.original_worth - receipt.net_received;
  CHECK_GE( king_tax_revenue_received, 0 );
  player.royal_money += king_tax_revenue_received;
}

wait<> show_treasure_receipt( TS& ts, Player const& player,
                              TreasureReceipt const& receipt ) {
  string const harbor_name =
      config_nation.nations[player.nation].harbor_city_name;
  string msg;
  switch( receipt.transport_mode ) {
    case e_treasure_transport_mode::player:
      msg = fmt::format(
          "Treasure worth {}{} reimbursed in {} yielding [{}{}] "
          "after {}% taxes witheld.",
          receipt.original_worth,
          config_text.special_chars.currency, harbor_name,
          receipt.net_received,
          config_text.special_chars.currency,
          receipt.kings_cut_percent );
      break;
    case e_treasure_transport_mode::king_with_charge:
      msg = fmt::format(
          "Treasure worth {}{} arrives in {}!  The crown has "
          "provided a reimbursement of [{}{}] after a [{}%] "
          "witholding.",
          receipt.original_worth,
          config_text.special_chars.currency, harbor_name,
          receipt.net_received,
          config_text.special_chars.currency,
          receipt.kings_cut_percent );
      break;
    case e_treasure_transport_mode::king_no_extra_charge:
      msg = fmt::format(
          "Treasure worth {}{} arrives in {}!  The crown has "
          "provided a reimbursement of [{}{}] after a [{}%] tax "
          "witholding.",
          receipt.original_worth,
          config_text.special_chars.currency, harbor_name,
          receipt.net_received,
          config_text.special_chars.currency,
          receipt.kings_cut_percent );
      break;
    case e_treasure_transport_mode::traveling_merchants:
      msg = fmt::format(
          "Treasure sold to traveling merchants for [{}{}].",
          receipt.original_worth,
          config_text.special_chars.currency );
      break;
  }
  co_await ts.gui.message_box( msg );
}

maybe<int> treasure_from_dwelling( SSConst const& ss,
                                   IRand& rand,
                                   Player const& player,
                                   Dwelling const& dwelling ) {
  e_tribe const tribe_type =
      ss.natives.tribe_for( dwelling.id ).type;
  e_native_level const level =
      config_natives.tribes[tribe_type].level;
  auto& conf = config_natives.treasure.yield[level];
  bool const has_cortes =
      player.fathers.has[e_founding_father::hernan_cortes];
  bool const capital = dwelling.is_capital;

  bool const should_get_treasure =
      capital || has_cortes ||
      rand.bernoulli( conf.probability );
  if( !should_get_treasure ) return nothing;

  double amount =
      rand.between_ints( conf.range.min, conf.range.max );

  if( capital )
    amount *= config_natives.treasure.capital_amount_scale;

  if( has_cortes )
    amount *= config_natives.treasure.cortes_amount_scale;

  return base::round_down_to_nearest_int_multiple(
      amount, conf.multiple );
}

} // namespace rn
