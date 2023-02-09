/****************************************************************
**equip.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-28.
*
* Description: Equipping and unequipping units from commodities.
*
*****************************************************************/
#include "equip.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "market.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/unit-composer.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Harbor
*****************************************************************/
vector<HarborEquipOption> harbor_equip_options(
    SSConst const& ss, Player const& player,
    UnitComposition const& unit_comp ) {
  static unordered_map<e_commodity, int> const commodity_store =
      [] {
        unordered_map<e_commodity, int> res;
        // Should be enough for any change.
        for( e_commodity comm : refl::enum_values<e_commodity> )
          res[comm] = 100;
        return res;
      }();
  vector<UnitTransformationResult> transformations =
      possible_unit_transformations( unit_comp,
                                     commodity_store );
  vector<HarborEquipOption> options;
  for( auto const& transformation : transformations ) {
    if( transformation.modifier_deltas.size() != 1 )
      // Only consider transformations that entail precisely one
      // modifier change, that way we ignore the ones where the
      // unit changes to itself (zero modifier changes) and where
      // e.g. a colonist changes to a dragoon (two modifier
      // changes, for muskets and horses).
      //
      // There are some legitimate transformations with zero mod-
      // ifier changes, e.g. when a pioneer that has 60 tools
      // gains 40 tools, but we don't support that because it
      // would make things more complicated and it is unlikely
      // that we need to support that on the harbor dock.
      continue;
    auto [modifier, modifier_delta] =
        *transformation.modifier_deltas.begin();
    if( !config_unit_type.composition.modifier_traits[modifier]
             .player_can_grant )
      // This will prevent us from e.g. granting "independence"
      // status to a veteran soldier and making it a continental
      // soldier.
      continue;
    HarborEquipOption option;
    option.new_comp       = transformation.new_comp;
    option.modifier       = modifier;
    option.modifier_delta = modifier_delta;
    // Check if there are any commodity deltas. There should be
    // at most one.
    maybe<Commodity> comm;
    for( auto [comm_type, quantity] :
         transformation.commodity_deltas ) {
      if( quantity == 0 ) continue;
      // This should not trigger because we've already filtered
      // out the transformations that involve multiple modifiers,
      // and that should also filter out the ones that involve
      // multiple commodities.
      CHECK( !comm.has_value(),
             "multiple non-zero commodity deltas in unit "
             "transformation results." );
      CHECK( ( quantity < 0 ) !=
             ( option.modifier_delta ==
               e_unit_type_modifier_delta::del ) );
      CHECK( ( quantity > 0 ) !=
             ( option.modifier_delta ==
               e_unit_type_modifier_delta::add ) );
      // Note abs on quantity.
      comm = Commodity{ .type     = comm_type,
                        .quantity = abs( quantity ) };
    }
    if( comm.has_value() ) {
      if( player.old_world.market.commodities[comm->type]
              .boycott )
        continue;
      // The invoice function expects this.
      option.commodity_delta = comm;
      // Note: immediate price change allowance flag should be
      // irrelevant here.
      Invoice const invoice = transaction_invoice(
          ss, player, *comm,
          ( option.modifier_delta ==
            e_unit_type_modifier_delta::add )
              ? e_transaction::buy
              : e_transaction::sell,
          e_immediate_price_change_allowed::allowed );
      option.money_delta = invoice.money_delta_final;
      option.can_afford =
          ( player.money + option.money_delta >= 0 );
    } else {
      // The player can always afford it if it involves no com-
      // modities.
      option.can_afford = true;
    }
    options.push_back( std::move( option ) );
  }
  return options;
}

// Returns markup text with the description of the action.
string harbor_equip_description(
    HarborEquipOption const& option ) {
  string res;
  switch( option.modifier_delta ) {
    case e_unit_type_modifier_delta::add:
      switch( option.modifier ) {
        case e_unit_type_modifier::blessing:
          res = "Bless as [Missionary]";
          break;
        case e_unit_type_modifier::horses:
          res = "Equip with [Horses]";
          break;
        case e_unit_type_modifier::muskets:
          res = "Arm with [Muskets]";
          break;
        case e_unit_type_modifier::tools: {
          // Unlike when unequipping tools (which could be an ar-
          // bitrary amount (whatever the pioneer has), when we
          // are equipping we only support equipping 100, since
          // equipping less than that would make things too com-
          // plicated.
          UNWRAP_CHECK( comm, option.commodity_delta );
          CHECK( comm.type == e_commodity::tools );
          CHECK( comm.quantity == 100 );
          res = "Equip with [Tools]";
          break;
        }
        case e_unit_type_modifier::independence:
          SHOULD_NOT_BE_HERE;
        case e_unit_type_modifier::strength: //
          SHOULD_NOT_BE_HERE;
      }
      break;
    case e_unit_type_modifier_delta::del:
      switch( option.modifier ) {
        case e_unit_type_modifier::blessing:
          res = "Cancel [Missionary] status";
          break;
        case e_unit_type_modifier::horses:
          res = "Sell [Horses]";
          break;
        case e_unit_type_modifier::muskets:
          res = "Sell [Muskets]";
          break;
        case e_unit_type_modifier::tools: {
          UNWRAP_CHECK( comm, option.commodity_delta );
          CHECK( comm.type == e_commodity::tools );
          CHECK( comm.quantity > 0 );
          res = fmt::format( "Sell [{} Tools]",
                             comm.quantity );
          break;
        }
        case e_unit_type_modifier::independence:
          SHOULD_NOT_BE_HERE;
        case e_unit_type_modifier::strength: //
          SHOULD_NOT_BE_HERE;
      }
      break;
  }
  if( option.money_delta < 0 )
    res +=
        fmt::format( " (costs [{}])", -option.money_delta );
  else if( option.money_delta > 0 )
    res +=
        fmt::format( " (save [{}])", option.money_delta );
  res += '.';
  return res;
}

PriceChange perform_harbor_equip_option(
    SS& ss, Player& player, UnitId unit_id,
    HarborEquipOption const& option ) {
  PriceChange price_change = {};
  Unit&       unit         = ss.units.unit_for( unit_id );
  unit.change_type( player, option.new_comp );
  if( option.commodity_delta.has_value() ) {
    Invoice const invoice = transaction_invoice(
        ss, player, *option.commodity_delta,
        ( option.modifier_delta ==
          e_unit_type_modifier_delta::add )
            ? e_transaction::buy
            : e_transaction::sell,
        e_immediate_price_change_allowed::allowed );
    CHECK( option.money_delta == invoice.money_delta_final );
    apply_invoice( ss, player, invoice );
    price_change = invoice.price_change;
  }
  return price_change;
}

/****************************************************************
** Colony
*****************************************************************/
vector<ColonyEquipOption> colony_equip_options(
    Colony const& colony, UnitComposition const& unit_comp ) {
  unordered_map<e_commodity, int> const commodity_store = [&] {
    unordered_map<e_commodity, int> res;
    // Should be enough for any change.
    for( e_commodity comm : refl::enum_values<e_commodity> )
      res[comm] = colony.commodities[comm];
    return res;
  }();
  vector<UnitTransformationResult> const transformations =
      possible_unit_transformations( unit_comp,
                                     commodity_store );
  vector<ColonyEquipOption> options;
  for( auto const& transformation : transformations ) {
    for( auto& [modifier, modifier_delta] :
         transformation.modifier_deltas ) {
      if( !config_unit_type.composition.modifier_traits[modifier]
               .player_can_grant )
        // This will prevent us from e.g. granting "independence"
        // status to a veteran soldier and making it a conti-
        // nental soldier.
        goto skip;
      if( modifier == e_unit_type_modifier::blessing &&
          !building_for_slot(
              colony, e_colony_building_slot::crosses ) ) {
        // We cannot bless units as missionaries unless the
        // colony has a church/cathedral.
        goto skip;
      }
    }
    {
      ColonyEquipOption option;
      option.new_comp = transformation.new_comp;
      for( auto [comm, q] : transformation.commodity_deltas )
        option.commodity_deltas[comm] = q;
      options.push_back( std::move( option ) );
    }
  skip:;
  }
  return options;
}

string colony_equip_description(
    ColonyEquipOption const& option ) {
  return config_unit_type.composition
      .unit_types[option.new_comp.type()]
      .name;
}

void perform_colony_equip_option(
    Colony& colony, Player const& player, Unit& unit,
    ColonyEquipOption const& option ) {
  unit.change_type( player, option.new_comp );
  for( auto& [comm, delta] : option.commodity_deltas ) {
    colony.commodities[comm] += delta;
    CHECK_GE( colony.commodities[comm], 0 );
  }
}

} // namespace rn
