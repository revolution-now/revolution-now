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
#include "colony-buildings.hpp"
#include "colony.hpp"
#include "gs-units.hpp"
#include "land-production.hpp"
#include "player.hpp"
#include "unit.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/production.rds.hpp"

// refl
#include "refl/to-str.hpp"

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

[[nodiscard]] int apply_int_percent_bonus( int n, int percent ) {
  return n + int( lround( n * double( percent ) / 100.0 ) );
}

struct BuildingBonusResult {
  int use = 0;
  int put = 0;
};

// Given the amount of a product that is produced by units in the
// base level building, compute the amount that is theoretically
// produced and used given the actual building's bonus type.
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

// Given a certain amount of a raw material that will theoreti-
// cally be used, compute the amount of product produced.
[[nodiscard]] int raw_to_produced_for_bonus_type(
    int raw, BuildingBonus_t const& bonus ) {
  switch( bonus.to_enum() ) {
    case BuildingBonus::e::none: return raw;
    case BuildingBonus::e::same: return raw;
    case BuildingBonus::e::factory: {
      auto const& o = bonus.get<BuildingBonus::factory>();
      // For the original game, the use_bonus is 100% and the put
      // bonus is 200%, which means that the output is 50% larger
      // than the input (counting the starting quantity, which is
      // 100% by definition). So in that case, percent_increase
      // will be 50.
      int percent_increase = ( ( o.put_bonus + 100 ) * 100 ) /
                                 ( o.use_bonus + 100 ) -
                             100;
      return apply_int_percent_bonus( raw, percent_increase );
    }
  }
}

int crosses_production_for_colony( UnitsState const& units_state,
                                   Player const&     player,
                                   Colony const&     colony ) {
  int const base_quantity = config_production.base_crosses;

  maybe<e_colony_building> maybe_building = building_for_slot(
      colony, e_colony_building_slot::crosses );
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

// This will compute all of the fields in the RawMaterialAnd-
// Product except for the product_delta_final, since computing
// the latter requires knowing the maximum capacity for a product
// in the colony which is computed differently depending on
// whether it is hammers or not.
void compute_raw_and_product_impl(
    Colony const& colony, TerrainState const& terrain_state,
    UnitsState const& units_state, e_outdoor_job outdoor_job,
    e_indoor_job indoor_job, e_commodity raw_commodity,
    RawMaterialAndProduct& out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( maybe<OutdoorUnit> const& unit =
            colony.outdoor_jobs()[d];
        unit.has_value() && unit->job == outdoor_job ) {
      int const quantity = production_on_square(
          outdoor_job, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location().moved( d ) );
      out.raw_produced += quantity;
      out_land_production[d] = SquareProduction{
          .what = outdoor_job, .quantity = quantity };
    }
  }

  // TODO: take into account center square here.

  int const product_produced_no_bonus = [&] {
    int res = 0;
    for( UnitId unit_id : colony.indoor_jobs()[indoor_job] )
      res += production_for_indoor_job(
          indoor_job, units_state.unit_for( unit_id ).type() );
    return res;
  }();

  e_colony_building_slot const building_slot =
      slot_for_indoor_job( indoor_job );
  maybe<e_colony_building> const building =
      building_for_slot( colony, building_slot );
  BuildingBonus_t const bonus =
      building.has_value()
          ? config_production
                .building_production_bonus[*building]
          : BuildingBonus::none{};
  BuildingBonusResult const bonus_res =
      apply_building_bonus( product_produced_no_bonus, bonus );
  out.product_produced_theoretical = bonus_res.put;
  out.raw_consumed_theoretical     = bonus_res.use;
  out.raw_delta_theoretical =
      out.raw_produced - out.raw_consumed_theoretical;

  // The quantities actually produced and consumed might have to
  // be lowered from their theoretical values if there isn't
  // enough total supply of the raw material. This is nontrivial
  // because of factory-level buildings.
  int const available_raw_input =
      out.raw_produced + colony.commodities()[raw_commodity];
  out.raw_consumed_actual = std::min(
      out.raw_consumed_theoretical, available_raw_input );
  out.product_produced_actual = raw_to_produced_for_bonus_type(
      out.raw_consumed_actual, bonus );

  int const warehouse_capacity =
      colony_warehouse_capacity( colony );

  int const current_raw_quantity =
      colony.commodities()[raw_commodity];
  int const proposed_raw_quantity = current_raw_quantity +
                                    out.raw_produced -
                                    out.raw_consumed_actual;
  out.raw_delta_final =
      std::min( proposed_raw_quantity, warehouse_capacity ) -
      current_raw_quantity;
  CHECK( out.raw_delta_final + current_raw_quantity >= 0,
         "colony supply of {} has gone negative ({}).",
         indoor_job,
         out.raw_delta_final + current_raw_quantity );

  // !! Note that product_delta_final has not yet been computed;
  // that must be done by the caller.
}

void compute_raw_and_product_generic(
    Colony const& colony, TerrainState const& terrain_state,
    UnitsState const& units_state, e_outdoor_job outdoor_job,
    e_indoor_job indoor_job, e_commodity raw_commodity,
    e_commodity product_commodity, RawMaterialAndProduct& out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  compute_raw_and_product_impl(
      colony, terrain_state, units_state, outdoor_job,
      indoor_job, raw_commodity, out, out_land_production );
  int const warehouse_capacity =
      colony_warehouse_capacity( colony );
  int const current_product_quantity =
      colony.commodities()[product_commodity];
  int const proposed_product_quantity =
      current_product_quantity + out.product_produced_actual;
  out.product_delta_final =
      std::min( proposed_product_quantity, warehouse_capacity ) -
      current_product_quantity;
  CHECK( out.product_delta_final >= 0,
         "product_delta_final for {} is negative ({}).",
         product_commodity, out.product_delta_final );
}

void compute_lumber_hammers(
    Colony const& colony, TerrainState const& terrain_state,
    UnitsState const& units_state, RawMaterialAndProduct& out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  compute_raw_and_product_impl(
      colony, terrain_state, units_state, e_outdoor_job::lumber,
      e_indoor_job::hammers, e_commodity::lumber, out,
      out_land_production );

  // There are no limits on the number of hammers it seems.
  out.product_delta_final = out.product_produced_actual;
  CHECK( out.product_delta_final >= 0,
         "product_delta_final for hammers is negative ({}).",
         out.product_delta_final );
}

void compute_land_production( ColonyProduction&   pr,
                              Colony const&       colony,
                              TerrainState const& terrain_state,
                              UnitsState const&   units_state ) {
  auto generic = [&]( e_outdoor_job          outdoor_job,
                      e_indoor_job           indoor_job,
                      e_commodity            raw_commodity,
                      e_commodity            product_commodity,
                      RawMaterialAndProduct& out ) {
    return compute_raw_and_product_generic(
        colony, terrain_state, units_state, outdoor_job,
        indoor_job, raw_commodity, product_commodity, out,
        pr.land_production );
  };

  // Sugar+Rum.
  generic( e_outdoor_job::sugar, e_indoor_job::rum,
           e_commodity::sugar, e_commodity::rum, pr.sugar_rum );

  // Tobacco+Cigars.
  generic( e_outdoor_job::tobacco, e_indoor_job::cigars,
           e_commodity::tobacco, e_commodity::cigars,
           pr.tobacco_cigars );

  // Cotton+Cloth.
  generic( e_outdoor_job::cotton, e_indoor_job::cloth,
           e_commodity::cotton, e_commodity::cloth,
           pr.cotton_cloth );

  // Fur+Coats.
  generic( e_outdoor_job::fur, e_indoor_job::coats,
           e_commodity::fur, e_commodity::coats, pr.fur_coats );

  // Lumber+Hammers.
  compute_lumber_hammers( colony, terrain_state, units_state,
                          pr.lumber_hammers,
                          pr.land_production );

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

  compute_land_production( res, colony, terrain_state,
                           units_state );

  return res;
}

} // namespace rn
