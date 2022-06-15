/****************************************************************
**production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-31.
*
* Description: Computes what is produced by a colony.
*
*****************************************************************/
#include "production.hpp"

// Revolution Now
#include "colony.hpp"
#include "gs-units.hpp"
#include "player.hpp"
#include "unit.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/production.rds.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {

// Note that these numbers will be different from outdoor jobs.
// For example, a petty criminal is as good as a free colonist at
// farming.
int production_for_indoor_job( e_indoor_job job,
                               e_unit_type  type ) {
  switch( type ) {
    case e_unit_type::petty_criminal:
      return config_production.indoor_production
          .petty_criminal_base_production;
    case e_unit_type::indentured_servant:
      return config_production.indoor_production
          .indentured_servant_base_production;
    case e_unit_type::free_colonist:
      return config_production.indoor_production
          .non_expert_base_production;
    default: break;
  }
  auto switch_expert = [&]( e_unit_type expert ) {
    if( type == expert )
      return config_production.indoor_production
          .expert_base_production;
    return config_production.indoor_production
        .non_expert_base_production;
  };
  switch( job ) {
    case e_indoor_job::bells:
      return switch_expert( e_unit_type::elder_statesman );
    case e_indoor_job::crosses:
      return switch_expert( e_unit_type::firebrand_preacher );
    case e_indoor_job::hammers:
      return switch_expert( e_unit_type::master_carpenter );
    case e_indoor_job::rum:
      return switch_expert( e_unit_type::master_rum_distiller );
    case e_indoor_job::cigars:
      return switch_expert( e_unit_type::master_tobacconist );
    case e_indoor_job::cloth:
      return switch_expert( e_unit_type::master_weaver );
    case e_indoor_job::coats:
      return switch_expert( e_unit_type::master_fur_trader );
    case e_indoor_job::tools:
      return switch_expert( e_unit_type::master_blacksmith );
    case e_indoor_job::muskets:
      return switch_expert( e_unit_type::master_gunsmith );
    case e_indoor_job::teacher: SHOULD_NOT_BE_HERE;
  }
}

maybe<e_colony_building> find_highest_building(
    refl::enum_map<e_colony_building, bool> const& buildings,
    e_colony_building level0, e_colony_building level1 ) {
  if( buildings[level1] ) return level1;
  if( buildings[level0] ) return level0;
  return nothing;
}

[[nodiscard]] int apply_int_percent_bonus( int n, int percent ) {
  return n + int( lround( n * double( percent ) / 100.0 ) );
}

struct BuildingBonusResult {
  int use = 0;
  int put = 0;
};

[[nodiscard]] BuildingBonusResult apply_building_bonus(
    int n, BuildingBonus_t const& bonus ) {
  switch( bonus.to_enum() ) {
    case BuildingBonus::e::none: {
      return BuildingBonusResult{ .use = n, .put = n };
    }
    case BuildingBonus::e::same: {
      auto const& o = bonus.get<BuildingBonus::same>();
      int         b = apply_int_percent_bonus( n, o.bonus );
      return BuildingBonusResult{ .use = b, .put = b };
    }
    case BuildingBonus::e::factory: {
      auto const& o = bonus.get<BuildingBonus::factory>();
      int use       = apply_int_percent_bonus( n, o.use_bonus );
      int put       = apply_int_percent_bonus( n, o.put_bonus );
      return BuildingBonusResult{ .use = use, .put = put };
    }
  }
}

int crosses_production_for_colony( UnitsState const& units_state,
                                   Player const&     player,
                                   Colony const&     colony ) {
  int const base_quantity = config_production.base_crosses;

  maybe<e_colony_building> maybe_building =
      find_highest_building( colony.buildings(),
                             e_colony_building::church,
                             e_colony_building::cathedral );
  if( !maybe_building.has_value() ) {
    // If we have no relevant buildings then we're left with the
    // base value. Sanity check and leave.
    CHECK( colony.indoor_jobs()[e_indoor_job::crosses].size() ==
           0 );
    return base_quantity;
  }

  e_colony_building building = *maybe_building;

  // First let's get the base amount for the buildings.
  int const buildings_quantity =
      config_production.free_building_production[building];

  // Now let's get the quantity added by the units.
  vector<UnitId> const& unit_ids =
      colony.indoor_jobs()[e_indoor_job::crosses];
  int units_quantity = 0;
  for( UnitId id : unit_ids )
    units_quantity += production_for_indoor_job(
        e_indoor_job::crosses,
        units_state.unit_for( id ).type() );

  // Now add in any bonuses due to building upgrade. Note that we
  // are applying this to the units_quantity and not the total
  // cumulative quantity since that is what the original game
  // seems to do for crosses.
  if( maybe<BuildingBonus_t> const& bonus =
          config_production.building_production_bonus[building];
      bonus.has_value() ) {
    BuildingBonusResult res =
        apply_building_bonus( units_quantity, *bonus );
    // Producing crosses does not consume anything, so these
    // should not differ.
    CHECK( res.use == res.put );
    units_quantity = res.put;
  }

  // William Penn increases cross production by 50%, but this is
  // applied to the units production (not including base).
  if( player.fathers.has[e_founding_father::william_penn] )
    units_quantity =
        apply_int_percent_bonus( units_quantity, 50 );

  int total =
      base_quantity + buildings_quantity + units_quantity;

  return total;
}

void compute_food_production( Colony const&   colony,
                              FoodProduction& pr ) {
  // TODO

  // After all is said and done, if the colony will have enough
  // food, then we can potentially produce a new colonist.
  int const food_after_production =
      colony.commodities()[e_commodity::food] +
      pr.food_delta_final;
  int const food_needed_for_creation =
      config_colony.food_for_creating_new_colonist;
  if( food_after_production >= food_needed_for_creation ) {
    pr.food_consumed_by_new_colonist = food_needed_for_creation;
    pr.food_delta_final -= food_needed_for_creation;
    pr.colonist_created = true;
  }

  // One final sanity check.
  CHECK_GE( colony.commodities()[e_commodity::food] +
                pr.food_delta_final,
            0 );
}

void compute_land_production(
    ColonyProduction& pr, Colony const& colony,
    TerrainState const& terrain_state ) {
  // TODO
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColonyProduction production_for_colony(
    TerrainState const& terrain_state,
    UnitsState const& units_state, Player const& player,
    Colony const& colony ) {
  ColonyProduction res;

  res.crosses = crosses_production_for_colony( units_state,
                                               player, colony );
  // TODO: factor in sons of liberty bonuses and/or tory penalty.

  compute_food_production( colony, res.food );

  compute_land_production( res, colony, terrain_state );

  return res;
}

} // namespace rn
