/****************************************************************
**raid-effects.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-04-17.
*
* Description: Handles when braves raid a colony.
*
*****************************************************************/
#include "raid-effects.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "damaged.hpp"
#include "ieuro-agent.hpp"
#include "rand-enum.hpp"
#include "tribe-arms.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/commodity.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/text.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

BraveAttackColonyEffect choose_stolen_commodity(
    IRand& rand, Colony const& colony ) {
  auto const& conf = config_natives.combat.colony_attack;
  vector<e_commodity> stealable;
  stealable.reserve( refl::enum_count<e_commodity> );
  for( auto [comm, q] : colony.commodities )
    if( q >= conf.min_commodity_quantity_for_stealing )
      stealable.push_back( comm );
  if( stealable.empty() ) return BraveAttackColonyEffect::none{};
  e_commodity const type      = rand.pick_one( stealable );
  int const quantity_in_store = colony.commodities[type];
  // Upper bound.
  int comm_stolen_max = lround(
      conf.commodity_percent_stolen.max * quantity_in_store );
  if( comm_stolen_max <
      conf.min_commodity_quantity_for_stealing )
    comm_stolen_max = conf.min_commodity_quantity_for_stealing;
  CHECK_LE( comm_stolen_max, quantity_in_store );
  // Lower bound.
  int comm_stolen_min = lround(
      conf.commodity_percent_stolen.min * quantity_in_store );
  if( comm_stolen_min <
      conf.min_commodity_quantity_for_stealing )
    comm_stolen_min = conf.min_commodity_quantity_for_stealing;
  if( comm_stolen_min > comm_stolen_max )
    comm_stolen_min = comm_stolen_max;
  CHECK_LE( comm_stolen_min, comm_stolen_max );
  int const quantity_to_steal =
      rand.between_ints( comm_stolen_min, comm_stolen_max );
  CHECK_LE( quantity_to_steal, quantity_in_store );
  Commodity const commodity{ .type     = type,
                             .quantity = quantity_to_steal };
  return BraveAttackColonyEffect::commodity_stolen{
    .what = commodity };
}

BraveAttackColonyEffect calculate_money_stolen(
    SSConst const& ss, IRand& rand, Colony const& colony ) {
  static const auto none = BraveAttackColonyEffect::none{};
  // The SG states that, although undocumented, a colony with a
  // stockade (or higher it is assumed) will not have money
  // stolen during native raids.
  if( colony_has_building_level( colony,
                                 e_colony_building::stockade ) )
    return none;
  Player const& player =
      player_for_player_or_die( ss.players, colony.player );
  int const money = player.money;
  if( money == 0 ) return none;
  auto const& conf = config_natives.combat.colony_attack;
  int const money_stolen_max = std::min(
      static_cast<int>(
          lround( conf.money_percent_stolen.max * money ) ),
      conf.money_stolen_abs_range_max );
  int const money_stolen_min = std::min(
      static_cast<int>(
          lround( conf.money_percent_stolen.min * money ) ),
      money_stolen_max );
  CHECK_LE( money_stolen_min, money_stolen_max );
  if( money_stolen_max == 0 ) return none;
  int const quantity =
      rand.between_ints( money_stolen_min, money_stolen_max );
  if( quantity == 0 ) return none;
  CHECK_GT( quantity, 0 );
  return BraveAttackColonyEffect::money_stolen{ .quantity =
                                                    quantity };
}

BraveAttackColonyEffect choose_ship_to_damage(
    SSConst const& ss, IRand& rand, Colony const& colony ) {
  // The SG states that, although undocumented, a colony with a
  // fortress won't have ships damaged during native raids.
  if( colony_has_building_level( colony,
                                 e_colony_building::fortress ) )
    return BraveAttackColonyEffect::none{};
  unordered_set<GenericUnitId> const& units =
      ss.units.from_coord( colony.location );
  vector<UnitId> ships;
  for( GenericUnitId const generic_id : units ) {
    UnitId const unit_id =
        ss.units.check_euro_unit( generic_id );
    Unit const& unit = ss.units.unit_for( unit_id );
    if( !unit.desc().ship ) continue;
    if( unit.orders().holds<unit_orders::damaged>() ) continue;
    ships.push_back( unit_id );
  }
  if( ships.empty() ) return BraveAttackColonyEffect::none{};
  // Need this for determinism; the units are ultimately taken
  // from an unordered set, so will be in random order.
  std::sort( ships.begin(), ships.end() );
  UnitId const ship = rand.pick_one( ships );

  maybe<ShipRepairPort> const port = find_repair_port_for_ship(
      ss, colony.player, colony.location );
  return BraveAttackColonyEffect::ship_in_port_damaged{
    .which = ship, .sent_to = port };
}

BraveAttackColonyEffect choose_building_to_destroy(
    IRand& rand, Colony const& colony ) {
  auto const& conf       = config_natives.combat.colony_attack;
  static const auto none = BraveAttackColonyEffect::none{};
  // The SG states that, although undocumented, a colony with a
  // fort (or fortress, it is assumed) will not have any build-
  // ings destroyed during native raids.
  if( colony_has_building_level( colony,
                                 e_colony_building::fort ) )
    return none;

  // First randomly choose a building slot.
  e_colony_building_slot const slot =
      pick_one<e_colony_building_slot>( rand );
  if( !conf.building_slots_eligible_for_destruction[slot] )
    return none;

  maybe<e_colony_building> const building =
      building_for_slot( colony, slot );
  if( !building.has_value() ) return none;
  // The OG does not destroy any of the initial colony buildings.
  if( config_colony.initial_colony_buildings[*building] )
    return none;
  return BraveAttackColonyEffect::building_destroyed{
    .which = *building };
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
// Not 100% sure on this, but experiments on the OG seem to indi-
// cate that the general strategy for selecting an outcome is
// that it will select the complete outcome before checking if it
// is possible; if the selected outcome is not possible then it
// will just return none{}. This would be as opposed to a priori
// adjustments to the weights (i.e., zero-ing some out) when a
// particular outcome would not make sense.
BraveAttackColonyEffect select_brave_attack_colony_effect(
    SSConst const& ss, IRand& rand, Colony const& colony ) {
  auto const& conf = config_natives.combat.colony_attack;
  e_brave_attack_colony_effect const effect =
      rand.pick_from_weighted_values( conf.outcome_weights );
  switch( effect ) {
    case e_brave_attack_colony_effect::none:
      return BraveAttackColonyEffect::none{};
    case e_brave_attack_colony_effect::commodity_stolen:
      return choose_stolen_commodity( rand, colony );
    case e_brave_attack_colony_effect::money_stolen:
      return calculate_money_stolen( ss, rand, colony );
    case e_brave_attack_colony_effect::building_destroyed:
      return choose_building_to_destroy( rand, colony );
    case e_brave_attack_colony_effect::ship_in_port_damaged:
      return choose_ship_to_damage( ss, rand, colony );
  }
}

void perform_brave_attack_colony_effect(
    SS& ss, TS& ts, Colony& colony, Tribe& tribe,
    BraveAttackColonyEffect const& effect ) {
  SWITCH( effect ) {
    CASE( none ) { return; }
    CASE( commodity_stolen ) {
      Commodity const& what = commodity_stolen.what;
      CHECK_GE( colony.commodities[what.type], what.quantity );
      switch( what.type ) {
        case rn::e_commodity::muskets:
          acquire_muskets_from_colony_raid( tribe,
                                            what.quantity );
          break;
        case rn::e_commodity::horses:
          acquire_horses_from_colony_raid( ss, tribe,
                                           what.quantity );
          break;
        default:
          break;
      }
      colony.commodities[what.type] -= what.quantity;
      // NOTE: the stolen commodity never goes to the tribe's
      // stock.
      return;
    }
    CASE( money_stolen ) {
      Player& player =
          player_for_player_or_die( ss.players, colony.player );
      CHECK_GE( player.money, money_stolen.quantity );
      player.money -= money_stolen.quantity;
      return;
    }
    CASE( building_destroyed ) {
      e_colony_building const building =
          building_destroyed.which;
      CHECK( colony.buildings[building] );
      // In the OG, when a a building is destroyed, all lower
      // buildings in that slot remain. So e.g. if a Cathedral is
      // destroyed, the Church will remain. If a Shipyard is de-
      // stroyed, the Drydock will remain. So setting this one
      // building to false should do the right thing, under the
      // assumption that, under normal game rules, the player
      // can't acquire one building without first having the one
      // below it.
      colony.buildings[building] = false;
      return;
    }
    CASE( ship_in_port_damaged ) {
      UnitId const ship_id = ship_in_port_damaged.which;
      Unit& ship           = ss.units.unit_for( ship_id );
      if( !ship_in_port_damaged.sent_to.has_value() ) {
        UnitOwnershipChanger( ss, ship_id ).destroy();
      } else {
        move_damaged_ship_for_repair(
            ss, ts.map_updater(), ship,
            *ship_in_port_damaged.sent_to );
      }
    }
  }
}

wait<> display_brave_attack_colony_effect_msg(
    SSConst const& ss, IEuroAgent& agent, Colony const& colony,
    BraveAttackColonyEffect const& effect, e_tribe tribe ) {
  SWITCH( effect ) {
    CASE( none ) { co_return; }
    CASE( commodity_stolen ) {
      co_await agent.message_box(
          "[{}] looting parties have stolen [{}] tons of [{}] "
          "from [{}]!",
          config_natives.tribes[tribe].name_possessive,
          commodity_stolen.what.quantity,
          config_commodity.types[commodity_stolen.what.type]
              .lowercase_display_name,
          colony.name );
      co_return;
    }
    CASE( money_stolen ) {
      co_await agent.message_box(
          "[{}] looting parties have stolen [{}{}] from the "
          "treasury!",
          config_natives.tribes[tribe].name_possessive,
          money_stolen.quantity,
          config_text.special_chars.currency );
      co_return;
    }
    CASE( building_destroyed ) {
      co_await agent.message_box(
          "[{}] raiding parties have destroyed the [{}] in "
          "[{}]!",
          config_natives.tribes[tribe].name_possessive,
          config_colony
              .building_display_names[building_destroyed.which],
          colony.name );
      co_return;
    }
    CASE( ship_in_port_damaged ) {
      UnitId const ship_id = ship_in_port_damaged.which;
      Unit const& ship     = ss.units.unit_for( ship_id );
      e_ship_damaged_reason const reason =
          e_ship_damaged_reason::battle;
      string msg;
      if( !ship_in_port_damaged.sent_to.has_value() )
        msg = ship_damaged_no_port_message(
            player_for_player_or_die( ss.players,
                                      ship.player_type() ),
            ship.type(), reason );
      else
        msg = ship_damaged_message(
            ss, ship.player_type(), ship.type(), reason,
            *ship_in_port_damaged.sent_to );
      int const num_units_onboard =
          ship.cargo().count_items_of_type<Cargo::unit>();
      // This is to ensure that we replicate the behavior of the
      // OG which does not have a concept of units on ships; it
      // just has sentried units whose square conincides with a
      // ship.
      CHECK( num_units_onboard == 0,
             "before a colony is destroyed, any units in the "
             "cargo of ships in its port must be removed." );
      co_await agent.message_box( "{}", msg );
      co_return;
    }
  }
}

} // namespace rn
