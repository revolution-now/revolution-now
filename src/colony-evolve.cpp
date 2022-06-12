/****************************************************************
**colony-evolve.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-04.
*
* Description: Evolves one colony one turn.
*
*****************************************************************/
#include "colony-evolve.hpp"

// Revolution Now
#include "colony.hpp"
#include "on-map.hpp"
#include "player.hpp"
#include "production.hpp"
#include "rand.hpp"
#include "ustate.hpp"
#include "utype.hpp"

// config
#include "config/colony.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

// This will check for spoilage given warehouse capacity. If any-
// thing spoils then it will be removed from the commodity store
// and a notification will be returned.
maybe<ColonyNotification::spoilage> check_spoilage(
    Colony& colony ) {
  int const food_capacity =
      config_colony.warehouses.food_max_quantity;
  int const warehouse_capacity =
      colony.buildings()[e_colony_building::warehouse_expansion]
          ? config_colony.warehouses
                .warehouse_expansion_max_quantity
      : colony.buildings()[e_colony_building::warehouse]
          ? config_colony.warehouses.warehouse_max_quantity
          : config_colony.warehouses.default_max_quantity;

  vector<Commodity>                 spoiled;
  refl::enum_map<e_commodity, int>& commodities =
      colony.commodities();
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    int const max = ( c == e_commodity::food )
                        ? food_capacity
                        : warehouse_capacity;
    if( commodities[c] > max ) {
      int spoilage = commodities[c] - max;
      spoiled.push_back(
          Commodity{ .type = c, .quantity = spoilage } );
      commodities[c] = max;
    }
  }

  if( spoiled.empty() ) return nothing;
  return ColonyNotification::spoilage{
      .spoiled = std::move( spoiled ) };
}

void apply_production_to_colony(
    Colony& colony, ColonyProduction const& production ) {
  refl::enum_map<e_commodity, int>& commodities =
      colony.commodities();

  commodities[e_commodity::food] +=
      production.food.food_delta_final;
  commodities[e_commodity::horses] +=
      production.food.horses_delta_final;

  commodities[e_commodity::sugar] +=
      production.sugar_rum.raw_delta_final;
  commodities[e_commodity::rum] +=
      production.sugar_rum.product_delta_final;

  commodities[e_commodity::tobacco] +=
      production.tobacco_cigars.raw_delta_final;
  commodities[e_commodity::cigars] +=
      production.tobacco_cigars.product_delta_final;

  commodities[e_commodity::cotton] +=
      production.cotton_cloth.raw_delta_final;
  commodities[e_commodity::cloth] +=
      production.cotton_cloth.product_delta_final;

  commodities[e_commodity::fur] +=
      production.fur_coats.raw_delta_final;
  commodities[e_commodity::coats] +=
      production.fur_coats.product_delta_final;

  commodities[e_commodity::lumber] +=
      production.lumber_hammers.raw_delta_final;
  colony.add_hammers(
      production.lumber_hammers.product_delta_final );

  commodities[e_commodity::silver] += production.silver;

  commodities[e_commodity::ore] +=
      production.ore_products.ore_delta_final;
  commodities[e_commodity::tools] +=
      production.ore_products.tools_delta_final;
  commodities[e_commodity::muskets] +=
      production.ore_products.muskets_delta_final;

  for( e_commodity c : refl::enum_values<e_commodity> ) {
    CHECK( commodities[c] >= 0,
           "colony {} has a negative quantity ({}) of {}.",
           colony.name(), commodities[c], c );
  }
}

} // namespace

ColonyEvolution evolve_colony_one_turn(
    Colony&     colony, SettingsState const&,
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, IMapUpdater& map_updater ) {
  ColonyEvolution ev;

  ev.production = production_for_colony(
      terrain_state, units_state, player, colony );

  apply_production_to_colony( colony, ev.production );

  if( ev.production.food.colonist_created ) {
    UnitType colonist =
        UnitType::create( e_unit_type::free_colonist );
    auto unit_id =
        create_unit( units_state, colony.nation(), colonist );
    unit_to_map_square_non_interactive(
        units_state, map_updater, unit_id, colony.location() );
    ev.notifications.push_back(
        ColonyNotification::new_colonist{ .id = unit_id } );
  }

  maybe<ColonyNotification::spoilage> spoilage_notification =
      check_spoilage( colony );
  if( spoilage_notification.has_value() )
    ev.notifications.push_back(
        std::move( *spoilage_notification ) );

  return ev;
}

} // namespace rn
