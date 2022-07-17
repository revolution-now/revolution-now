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
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "land-production.hpp"
#include "unit.hpp"

// ss
#include "ss/colony-enums.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

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

maybe<e_commodity> product_from_raw( e_commodity raw ) {
  switch( raw ) {
    case e_commodity::food: return nothing;
    case e_commodity::sugar: return e_commodity::rum;
    case e_commodity::tobacco: return e_commodity::cigars;
    case e_commodity::cotton: return e_commodity::cloth;
    case e_commodity::fur: return e_commodity::coats;
    case e_commodity::lumber: return nothing;
    case e_commodity::ore: return e_commodity::tools;
    case e_commodity::silver: return nothing;
    case e_commodity::horses: return nothing;
    case e_commodity::rum: return nothing;
    case e_commodity::cigars: return nothing;
    case e_commodity::cloth: return nothing;
    case e_commodity::coats: return nothing;
    case e_commodity::trade_goods: return nothing;
    case e_commodity::tools: return e_commodity::muskets;
    case e_commodity::muskets: return nothing;
  }
}

maybe<e_indoor_job> indoor_job_from_outdoor_job(
    e_outdoor_job job ) {
  switch( job ) {
    case e_outdoor_job::food: return nothing;
    case e_outdoor_job::fish: return nothing;
    case e_outdoor_job::sugar: return e_indoor_job::rum;
    case e_outdoor_job::tobacco: return e_indoor_job::cigars;
    case e_outdoor_job::cotton: return e_indoor_job::cloth;
    case e_outdoor_job::fur: return e_indoor_job::coats;
    case e_outdoor_job::lumber: return e_indoor_job::hammers;
    case e_outdoor_job::ore: return e_indoor_job::tools;
    case e_outdoor_job::silver: return nothing;
  }
}

// Note that these numbers will be different from outdoor jobs.
// For example, a petty criminal is as good as a free colonist at
// farming.
int production_for_indoor_job( e_indoor_job job,
                               e_unit_type  type ) {
  if( job == e_indoor_job::bells ) {
    // Bells seems to be an exception to the rule for indoor pro-
    // duction, so we need a special case for it. In particular,
    // it seems that a free colonist produces 5 bells, indentured
    // servant 3, and petty criminal/convert produces 2.
    TODO(
        "Need to make this mechanism more general so that we "
        "can specify production for each indoor job "
        "individually." );
  }
  switch( type ) {
    case e_unit_type::petty_criminal:
      return config_production.indoor_production
          .petty_criminal_base_production;
    case e_unit_type::native_convert:
      return config_production.indoor_production
          .native_convert_base_production;
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
    case e_indoor_job::bells: {
      // See above.
      SHOULD_NOT_BE_HERE;
    }
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
    CHECK( colony.indoor_jobs[e_indoor_job::crosses].size() ==
           0 );
    return base_quantity;
  }

  e_colony_building building = *maybe_building;

  // First let's get the base amount for the buildings.
  int const buildings_quantity =
      config_production.free_building_production[building];

  // Now let's get the quantity added by the units.
  vector<UnitId> const& unit_ids =
      colony.indoor_jobs[e_indoor_job::crosses];
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

void compute_food_production(
    TerrainState const& terrain_state,
    UnitsState const& units_state, Colony const& colony,
    int const center_food_produced, FoodProduction& out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( maybe<OutdoorUnit> const& unit = colony.outdoor_jobs[d];
        unit.has_value() && unit->job == e_outdoor_job::food ) {
      int const quantity = production_on_square(
          e_outdoor_job::food, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location.moved( d ) );
      out.corn_produced += quantity;
      out_land_production[d] = SquareProduction{
          .what = e_outdoor_job::food, .quantity = quantity };
    }
  }
  // This must have already been computed.
  out.corn_produced += center_food_produced;

  for( e_direction d : refl::enum_values<e_direction> ) {
    if( maybe<OutdoorUnit> const& unit = colony.outdoor_jobs[d];
        unit.has_value() && unit->job == e_outdoor_job::fish ) {
      int const quantity = production_on_square(
          e_outdoor_job::fish, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location.moved( d ) );
      out.fish_produced += quantity;
      out_land_production[d] = SquareProduction{
          .what = e_outdoor_job::fish, .quantity = quantity };
    }
  }

  out.food_produced = out.corn_produced + out.fish_produced;
  CHECK_GE( out.food_produced, 0 );

  int const population = colony_population( colony );
  out.food_consumed_by_colonists_theoretical = population * 2;
  CHECK_GE( out.food_consumed_by_colonists_theoretical, 0 );

  int const food_in_warehouse_before =
      colony.commodities[e_commodity::food];
  out.food_consumed_by_colonists_actual =
      ( food_in_warehouse_before + out.food_produced ) >=
              out.food_consumed_by_colonists_theoretical
          ? out.food_consumed_by_colonists_theoretical
          : ( food_in_warehouse_before + out.food_produced );
  CHECK_GE( out.food_consumed_by_colonists_actual, 0 );

  // Either zero or positive.
  out.food_deficit =
      ( food_in_warehouse_before + out.food_produced ) >=
              out.food_consumed_by_colonists_theoretical
          ? 0
          : out.food_consumed_by_colonists_theoretical -
                ( food_in_warehouse_before + out.food_produced );
  CHECK_GE( out.food_deficit, 0 );

  out.food_surplus_before_horses = std::max(
      0, out.food_produced -
             out.food_consumed_by_colonists_theoretical );

  int const warehouse_capacity =
      colony_warehouse_capacity( colony );

  // We must have at least two horses to breed. If we do, then we
  // produce one extra horse per 25 (or less) horses. I.e., 50
  // horses produces two horses per turn, and 51 produces three
  // per turn.
  int const current_horses =
      colony.commodities[e_commodity::horses];
  out.horses_produced_theoretical =
      ( current_horses < 2 ) ? 0 : ( current_horses + 24 ) / 25;
  if( colony.buildings[e_colony_building::stable] )
    out.horses_produced_theoretical *= 2;
  int const food_per_new_horse = 1;
  out.max_new_horses_allowed =
      out.food_surplus_before_horses / food_per_new_horse;

  // We have the opportunity to actually produce some horses
  // since we have some (non-warehouse) food surplus this turn.
  out.horses_produced_actual =
      std::min( out.max_new_horses_allowed,
                out.horses_produced_theoretical );
  out.food_consumed_by_horses =
      food_per_new_horse * out.horses_produced_actual;
  if( out.horses_produced_actual > 0 ||
      out.max_new_horses_allowed > 0 ) {
    CHECK( out.food_deficit == 0 );
  }

  int const proposed_new_horse_quantity =
      current_horses + out.horses_produced_actual;
  out.horses_delta_final = std::min( proposed_new_horse_quantity,
                                     warehouse_capacity ) -
                           current_horses;
  if( out.horses_delta_final < 0 ) {
    // Since horse quantities can never decrease due to food
    // shortages, if we are here then this means that we are over
    // warehouse capacity. We will therefore set the delta to
    // zero and let the spoilage detector (which happens sepa-
    // rately) remove the excess quantity.
    out.horses_delta_final = 0;
  }
  CHECK( out.horses_delta_final + current_horses >= 0,
         "colony supply of horses has gone negative ({}).",
         out.horses_delta_final + current_horses );

  // Do this again since it is important.
  CHECK_GE( out.food_deficit, 0 );

  if( out.food_deficit > 0 ) {
    CHECK( out.horses_produced_actual == 0 );
    // Final food delta can be computed without regard to horses.
    out.horses_delta_final               = 0;
    int const proposed_new_food_quantity = 0;
    // Since there are no warehouse limits on the amount of food,
    // we can just compute the final delta.
    out.food_delta_final = ( proposed_new_food_quantity -
                             food_in_warehouse_before );
    CHECK( food_in_warehouse_before + out.food_delta_final ==
           0 );
    out.colonist_starved = true;
  } else {
    // Final food delta must take into account horses.
    int const proposed_new_food_quantity =
        food_in_warehouse_before + out.food_produced -
        out.food_consumed_by_colonists_actual -
        out.food_consumed_by_horses;
    out.food_delta_final =
        proposed_new_food_quantity - food_in_warehouse_before;
    // Note that food_delta_final could be positive or negative
    // here, since the fact that we have no food deficit may just
    // mean that there was enough in the warehouse to draw from.
    CHECK( out.colonist_starved == false );
  }

  CHECK( out.food_delta_final + food_in_warehouse_before >= 0,
         "colony supply of food has gone negative ({}).",
         out.food_delta_final + food_in_warehouse_before );
  if( out.colonist_starved ) {
    CHECK( out.food_delta_final + food_in_warehouse_before ==
           0 );
  }
}

void compute_raw(
    Colony const& colony, TerrainState const& terrain_state,
    UnitsState const& units_state, e_outdoor_job outdoor_job,
    maybe<SquareProduction const&> center_secondary,
    RawMaterialAndProduct&         out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( maybe<OutdoorUnit> const& unit = colony.outdoor_jobs[d];
        unit.has_value() && unit->job == outdoor_job ) {
      int const quantity = production_on_square(
          outdoor_job, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location.moved( d ) );
      out.raw_produced += quantity;
      out_land_production[d] = SquareProduction{
          .what = outdoor_job, .quantity = quantity };
    }
  }

  if( center_secondary.has_value() &&
      center_secondary->what == outdoor_job )
    // This must have already been computed.
    out.raw_produced += center_secondary->quantity;

  // This may be ammended if there is a product produced from
  // this raw good.
  out.raw_delta_theoretical = out.raw_produced;
}

void compute_product( Colony const&          colony,
                      e_indoor_job           indoor_job,
                      UnitsState const&      units_state,
                      e_commodity            raw_commodity,
                      RawMaterialAndProduct& out ) {
  int const product_produced_no_bonus = [&] {
    int res = 0;
    for( UnitId unit_id : colony.indoor_jobs[indoor_job] )
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
  out.raw_delta_theoretical -= out.raw_consumed_theoretical;

  // The quantities actually produced and consumed might have to
  // be lowered from their theoretical values if there isn't
  // enough total supply of the raw material. This is nontrivial
  // because of factory-level buildings.
  //
  // Note that the colony commodities at this point will already
  // include the raw product this turn.
  int const available_raw_input =
      colony.commodities[raw_commodity];
  out.raw_consumed_actual = std::min(
      out.raw_consumed_theoretical, available_raw_input );
  out.product_produced_actual = raw_to_produced_for_bonus_type(
      out.raw_consumed_actual, bonus );
}

void compute_land_production( ColonyProduction& pr,
                              Colony const&     colony_pristine,
                              TerrainState const& terrain_state,
                              UnitsState const&   units_state ) {
  // FIXME: copying not optimal.
  Colony colony = colony_pristine;

  auto do_product =
      [&]( e_indoor_job job, e_commodity raw,
           RawMaterialAndProduct& raw_and_product ) {
        compute_product( colony, job, units_state, raw,
                         raw_and_product );
        maybe<e_commodity> product = product_from_raw( raw );
        // E.g. lumber won't have a commodity product (because
        // ham- mers are not a commodity).
        if( !product.has_value() ) return;
        colony.commodities[*product] +=
            raw_and_product.product_produced_actual;
      };

  auto compute = [&]( e_outdoor_job          outdoor_job,
                      RawMaterialAndProduct& raw_and_product ) {
    compute_raw( colony, terrain_state, units_state, outdoor_job,
                 pr.center_extra_production, raw_and_product,
                 pr.land_production );
    e_commodity const raw =
        commodity_for_outdoor_job( outdoor_job );
    colony.commodities[raw] += raw_and_product.raw_produced;
    maybe<e_indoor_job> indoor_job =
        indoor_job_from_outdoor_job( outdoor_job );
    if( indoor_job.has_value() )
      do_product( *indoor_job, raw, raw_and_product );
    colony.commodities[raw] -=
        raw_and_product.raw_consumed_actual;
  };

  compute( e_outdoor_job::sugar, pr.sugar_rum );
  compute( e_outdoor_job::tobacco, pr.tobacco_cigars );
  compute( e_outdoor_job::cotton, pr.cotton_cloth );
  compute( e_outdoor_job::fur, pr.fur_coats );
  compute( e_outdoor_job::lumber, pr.lumber_hammers );
  compute( e_outdoor_job::silver, pr.silver );

  // Ore/tools/muskets.
  {
    // First compute ore/tools as if muskets don't exist.
    compute( e_outdoor_job::ore, pr.ore_tools );
    // Boostrap the muskets calculation with the tools produced
    // from the ore/tools stage.
    pr.tools_muskets.raw_produced =
        pr.ore_tools.product_produced_actual;
    pr.tools_muskets.raw_delta_theoretical =
        pr.tools_muskets.raw_produced;
    do_product( e_indoor_job::muskets, e_commodity::tools,
                pr.tools_muskets );
    colony.commodities[e_commodity::tools] -=
        pr.tools_muskets.raw_consumed_actual;
  }

  compute_food_production( terrain_state, units_state, colony,
                           pr.center_food_production, pr.food,
                           pr.land_production );

  // Warehouse adjustments. Note that this must be done after all
  // production is computed because in some cases (e.g. tools)
  // one production pair might use the result product of another
  // pair as input, so we don't want to prematurely do the ware-
  // house capping otherwise we will accidentally discard tools
  // that could be used to produce muskets this turn.
  int const warehouse_capacity =
      colony_warehouse_capacity( colony_pristine );

  auto adjust_for_warehouse = [&]( e_commodity comm,
                                   int&        delta_final ) {
    if( colony_pristine.commodities[comm] <=
            warehouse_capacity &&
        colony.commodities[comm] > warehouse_capacity )
      colony.commodities[comm] = warehouse_capacity;
    else if( colony_pristine.commodities[comm] >
                 warehouse_capacity &&
             colony.commodities[comm] >
                 colony_pristine.commodities[comm] )
      colony.commodities[comm] =
          colony_pristine.commodities[comm];
    delta_final = colony.commodities[comm] -
                  colony_pristine.commodities[comm];
  };

  adjust_for_warehouse( e_commodity::sugar,
                        pr.sugar_rum.raw_delta_final );
  adjust_for_warehouse( e_commodity::rum,
                        pr.sugar_rum.product_delta_final );

  adjust_for_warehouse( e_commodity::tobacco,
                        pr.tobacco_cigars.raw_delta_final );
  adjust_for_warehouse( e_commodity::cigars,
                        pr.tobacco_cigars.product_delta_final );

  adjust_for_warehouse( e_commodity::cotton,
                        pr.cotton_cloth.raw_delta_final );
  adjust_for_warehouse( e_commodity::cloth,
                        pr.cotton_cloth.product_delta_final );

  adjust_for_warehouse( e_commodity::fur,
                        pr.fur_coats.raw_delta_final );
  adjust_for_warehouse( e_commodity::coats,
                        pr.fur_coats.product_delta_final );

  adjust_for_warehouse( e_commodity::ore,
                        pr.ore_tools.raw_delta_final );
  // For tools, its the one in tools/muskets that has the final
  // value.
  adjust_for_warehouse( e_commodity::tools,
                        pr.tools_muskets.raw_delta_final );
  adjust_for_warehouse( e_commodity::muskets,
                        pr.tools_muskets.product_delta_final );

  adjust_for_warehouse( e_commodity::silver,
                        pr.silver.raw_delta_final );

  adjust_for_warehouse( e_commodity::lumber,
                        pr.lumber_hammers.raw_delta_final );
}

void fill_in_center_square( SSConst const&    ss,
                            Colony const&     colony,
                            ColonyProduction& pr ) {
  MapSquare const& square =
      ss.terrain.square_at( colony.location );

  // Food.
  pr.center_food_production = food_production_on_center_square(
      square, ss.settings.difficulty );

  // Secondary good.
  maybe<e_outdoor_commons_secondary_job> center_secondary =
      choose_secondary_job( square, ss.settings.difficulty );
  if( !center_secondary.has_value() ) return;
  pr.center_extra_production = SquareProduction{
      .what = to_outdoor_job( *center_secondary ),
      // Note that this quantity must be checked by the functions
      // dedicated to individual goods in order to factor them in
      // to the total calculations.
      .quantity = commodity_production_on_center_square(
          *center_secondary, square, ss.settings.difficulty ) };
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<int> production_for_slot( ColonyProduction const& pr,
                                e_colony_building_slot  slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return pr.tools_muskets.product_produced_theoretical;
    case e_colony_building_slot::tools:
      return pr.ore_tools.product_produced_theoretical;
    case e_colony_building_slot::rum:
      return pr.sugar_rum.product_produced_theoretical;
    case e_colony_building_slot::cloth:
      return pr.cotton_cloth.product_produced_theoretical;
    case e_colony_building_slot::coats:
      return pr.fur_coats.product_produced_theoretical;
    case e_colony_building_slot::cigars:
      return pr.tobacco_cigars.product_produced_theoretical;
    case e_colony_building_slot::hammers:
      return pr.lumber_hammers.product_produced_theoretical;
    case e_colony_building_slot::town_hall: return pr.bells;
    case e_colony_building_slot::newspapers: return nothing;
    case e_colony_building_slot::schools: return nothing;
    case e_colony_building_slot::offshore: return nothing;
    case e_colony_building_slot::horses:
      return pr.food.horses_produced_theoretical;
    case e_colony_building_slot::wall: return nothing;
    case e_colony_building_slot::warehouses: return nothing;
    case e_colony_building_slot::crosses: return pr.crosses;
    case e_colony_building_slot::custom_house: return nothing;
  }
}

ColonyProduction production_for_colony( SSConst const& ss,
                                        Colony const&  colony ) {
  ColonyProduction res;
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );

  res.crosses =
      crosses_production_for_colony( ss.units, player, colony );
  // TODO: factor in sons of liberty bonuses and/or tory penalty.

  // Note that this must be done before processing any of the
  // other land squares.
  fill_in_center_square( ss, colony, res );

  compute_land_production( res, colony, ss.terrain, ss.units );

  return res;
}

} // namespace rn
