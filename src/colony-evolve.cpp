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
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "on-map.hpp"
#include "production.hpp"
#include "rand.hpp"
#include "ts.hpp"
#include "ustate.hpp"

// gs
#include "ss/players.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// config
#include "config/colony.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

struct Player;

namespace {

void check_ran_out_of_raw_materials( ColonyEvolution& ev ) {
  auto check =
      [&]( e_commodity what, e_indoor_job job,
           RawMaterialAndProduct const& raw_and_product ) {
        if( raw_and_product.raw_consumed_theoretical > 0 &&
            raw_and_product.raw_consumed_actual == 0 )
          // This means that the raw good could have had some
          // amount consumed, but none was actually consumed,
          // meaning that there was none produced and none in the
          // warehouse.
          ev.notifications.push_back(
              ColonyNotification::run_out_of_raw_material{
                  .what = what, .job = job } );
      };

  check( e_commodity::sugar, e_indoor_job::rum,
         ev.production.sugar_rum );
  check( e_commodity::tobacco, e_indoor_job::cigars,
         ev.production.tobacco_cigars );
  check( e_commodity::cotton, e_indoor_job::cloth,
         ev.production.cotton_cloth );
  check( e_commodity::fur, e_indoor_job::coats,
         ev.production.fur_coats );
  check( e_commodity::lumber, e_indoor_job::hammers,
         ev.production.lumber_hammers );
  check( e_commodity::ore, e_indoor_job::tools,
         ev.production.ore_tools );
  check( e_commodity::tools, e_indoor_job::muskets,
         ev.production.tools_muskets );
}

// This will check for spoilage given warehouse capacity. If any-
// thing spoils then it will be removed from the commodity store
// and a notification will be returned.
maybe<ColonyNotification::spoilage> check_spoilage(
    Colony& colony ) {
  int const warehouse_capacity =
      colony_warehouse_capacity( colony );

  vector<Commodity>                 spoiled;
  refl::enum_map<e_commodity, int>& commodities =
      colony.commodities;
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    if( !config_colony.warehouses
             .commodities_with_warehouse_limit[c] )
      continue;
    int const max = warehouse_capacity;
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

config::colony::construction_requirements materials_needed(
    Construction_t const& construction ) {
  switch( construction.to_enum() ) {
    using namespace Construction;
    case e::building: {
      auto& o = construction.get<building>();
      return config_colony.requirements_for_building[o.what];
    }
    case e::unit: {
      auto& o = construction.get<unit>();
      maybe<config::colony::construction_requirements> const&
          materials =
              config_colony.requirements_for_unit[o.type];
      CHECK( materials.has_value(),
             "a colony is constructing unit {}, but that unit "
             "is not buildable.",
             o.type );
      return *materials;
    }
  }
}

void check_create_or_starve_colonist(
    UnitsState& units_state, Colony& colony,
    ColonyProduction const&       pr,
    vector<ColonyNotification_t>& notifications,
    IMapUpdater&                  map_updater ) {
  if( pr.food.colonist_starved ) {
    vector<UnitId> const units_in_colony =
        colony_units_all( colony );
    CHECK( !units_in_colony.empty() );
    if( units_in_colony.size() == 1 ) {
      // In the original game it seems that colonies (even those
      // on arctic squares) will always produce a minimum of two
      // food in the center square, which can support one
      // colonist, and so in the original game we would never
      // have a colonist starve when the population is one. In
      // our case we don't want to commit to that food minimum in
      // the center square, and so we will allow the possibility
      // that we get to this code path. TODO: It seems that the
      // proper thing to do here would be to delete both the
      // colonist and the colony. But for now we will just have
      // mercy on the colonist and ignore it.
    } else {
      UnitId      unit_id = rng::pick_one( units_in_colony );
      e_unit_type type = units_state.unit_for( unit_id ).type();
      // Note that calling `destroy_unit` is not enough, we have
      // to remove it from the colony as well.
      remove_unit_from_colony( units_state, colony, unit_id );
      units_state.destroy_unit( unit_id );
      notifications.emplace_back(
          ColonyNotification::colonist_starved{ .type = type } );
    }
    // At this point we may as well return because we can't have
    // a new colonist created in the same turn as one starved.
    return;
  }

  // Check for a colonist created. After all is said and done, if
  // the colony will have enough food, then we can potentially
  // produce a new colonist.
  int&      current_food = colony.commodities[e_commodity::food];
  int const food_needed_for_creation =
      config_colony.food_for_creating_new_colonist;

  if( current_food < food_needed_for_creation ) return;

  current_food -= food_needed_for_creation;
  UnitId unit_id = create_free_unit(
      units_state, colony.nation,
      UnitType::create( e_unit_type::free_colonist ) );
  unit_to_map_square_non_interactive( units_state, map_updater,
                                      unit_id, colony.location );
  notifications.emplace_back(
      ColonyNotification::new_colonist{ .id = unit_id } );

  // One final sanity check.
  CHECK_GE( colony.commodities[e_commodity::food], 0 );
}

void check_construction( UnitsState&  units_state,
                         IMapUpdater& map_updater,
                         Colony& colony, ColonyEvolution& ev ) {
  if( !colony.construction.has_value() ) return;
  Construction_t const& construction = *colony.construction;

  // First check if it's a building that the colony already has.
  if( auto building =
          construction.get_if<Construction::building>();
      building.has_value() ) {
    if( colony.buildings[building->what] ) {
      ev.notifications.emplace_back(
          ColonyNotification::construction_already_finished{
              .what = construction } );
      return;
    }
  }

  auto const requirements = materials_needed( construction );

  if( colony_population( colony ) <
      requirements.minimum_population ) {
    ev.notifications.emplace_back(
        ColonyNotification::construction_lacking_population{
            .what = construction,
            .required_population =
                requirements.minimum_population } );
    return;
  }

  if( requirements.required_building.has_value() &&
      !colony_has_building_level(
          colony, *requirements.required_building ) ) {
    ev.notifications.emplace_back(
        ColonyNotification::construction_lacking_building{
            .what = construction,
            .required_building =
                *requirements.required_building } );
    return;
  }

  if( colony.hammers < requirements.hammers ) return;

  int const have_tools = colony.commodities[e_commodity::tools];

  if( colony.commodities[e_commodity::tools] <
      requirements.tools ) {
    ev.notifications.emplace_back(
        ColonyNotification::construction_missing_tools{
            .what       = construction,
            .have_tools = have_tools,
            .need_tools = requirements.tools } );
    return;
  }

  // In the original game, when a construction finishes it resets
  // the hammers to zero, even if we've accumulated more than we
  // need (waiting for tools). If we didn't do this then it might
  // not give the player an incentive to get the tools in a
  // timely manner, since they would never lose hammers. That
  // said, it does appear to allow hammers to accumulate after a
  // construction (while we are waiting for the player to change
  // construction) and those hammers can then be reused on the
  // next construction (to some extent, depending on difficulty
  // level).
  colony.hammers = 0;
  colony.commodities[e_commodity::tools] -= requirements.tools;
  CHECK_GE( colony.commodities[e_commodity::tools], 0 );

  ev.notifications.emplace_back(
      ColonyNotification::construction_complete{
          .what = construction } );

  switch( construction.to_enum() ) {
    using namespace Construction;
    case e::building: {
      auto& o                  = construction.get<building>();
      colony.buildings[o.what] = true;
      DCHECK( !ev.built.has_value() );
      ev.built = o.what;
      return;
    }
    case e::unit: {
      auto& o = construction.get<unit>();
      // We need a real map updater here because e.g. theoreti-
      // cally we could construct a unit that has a two-square
      // sighting radius and that might cause new squares to be-
      // come visible, which would have to be rendered. That
      // said, in the original game, no unit that can be con-
      // structed in this manner has a sighting radius of more
      // than one.
      create_unit_on_map_non_interactive(
          units_state, map_updater, colony.nation,
          UnitComposition::create( o.type ), colony.location );
      break;
    }
  }
}

void apply_commodity_increase(
    refl::enum_map<e_commodity, int>& store, e_commodity what,
    int delta, vector<ColonyNotification_t>& notifications ) {
  int const old_value = store[what];
  int const new_value = old_value + delta;
  if( old_value < 100 && new_value >= 100 )
    // Don't report food because food is not typically something
    // that gets loaded as cargo and traded. Typically the player
    // will want to allow it to grow beyond 100 to 200 (which it
    // can always do because there are no warehouse limits on
    // food) in order to produce a new colonist.
    if( what != e_commodity::food )
      notifications.emplace_back(
          ColonyNotification::full_cargo{ .what = what } );
  store[what] = new_value;
}

void apply_production_to_colony(
    Colony& colony, ColonyProduction const& production,
    vector<ColonyNotification_t>& notifications ) {
  refl::enum_map<e_commodity, int>& commodities =
      colony.commodities;

  for( e_commodity c : refl::enum_values<e_commodity> ) {
    int delta =
        final_production_delta_for_commodity( production, c );
    apply_commodity_increase( commodities, c, delta,
                              notifications );
  }

  colony.hammers +=
      production.lumber_hammers.product_delta_final;

  for( e_commodity c : refl::enum_values<e_commodity> ) {
    CHECK( commodities[c] >= 0,
           "colony {} has a negative quantity ({}) of {}.",
           colony.name, commodities[c], c );
  }
}

} // namespace

ColonyEvolution evolve_colony_one_turn( SS& ss, TS& ts,
                                        Colony& colony ) {
  ColonyEvolution ev;
  ev.production = production_for_colony( ss, colony );

  apply_production_to_colony( colony, ev.production,
                              ev.notifications );

  check_ran_out_of_raw_materials( ev );

  check_construction( ss.units, ts.map_updater, colony, ev );

  // Needs to be done after food deltas have been applied.
  check_create_or_starve_colonist(
      ss.units, colony, ev.production, ev.notifications,
      ts.map_updater );

  // NOTE: This should be done last, so that anything above that
  // could potentially consume commodities can do so before they
  // spoil.
  maybe<ColonyNotification::spoilage> spoilage_notification =
      check_spoilage( colony );
  if( spoilage_notification.has_value() )
    ev.notifications.emplace_back(
        std::move( *spoilage_notification ) );

  return ev;
}

} // namespace rn
