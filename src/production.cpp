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
#include "sons-of-liberty.hpp"
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
#include "config/unit-type.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** BellsModifiers
*****************************************************************/
struct BellsModifiers {
  void apply_outdoor( int& to ) const {
    if( to == 0 ) return;
    to += sons_of_liberty_bonus_outdoor;
    to -= tory_penalty_outdoor;
  }

  void apply_indoor( int& to ) const {
    if( to == 0 ) return;
    to += sons_of_liberty_bonus_indoor;
    to -= tory_penalty_indoor;
  }

  // Shouldn't really access these directly except to construct
  // this object; to apply the bonuses use the above apply
  // methods so that we ensure that the correct logic gets ap-
  // plied.
  int sons_of_liberty_bonus_outdoor = 0;
  int sons_of_liberty_bonus_indoor  = 0;
  int tory_penalty_outdoor          = 0;
  int tory_penalty_indoor           = 0;
};

/****************************************************************
** Helpers
*****************************************************************/
bool indoor_unit_is_expert( e_indoor_job job,
                            e_unit_type  unit_type ) {
  UnitTypeAttributes const& attr = unit_attr( unit_type );
  e_unit_activity const     activity =
      activity_for_indoor_job( job );
  return ( attr.expertise == activity );
}

// Mutable version, used only internally.
int& final_production_delta_for_commodity( ColonyProduction& pr,
                                           e_commodity c ) {
  switch( c ) {
    case e_commodity::food:
      return pr.food_horses.food_delta_final;
    case e_commodity::horses:
      return pr.food_horses.horses_delta_final;
    case e_commodity::sugar: return pr.sugar_rum.raw_delta_final;
    case e_commodity::rum:
      return pr.sugar_rum.product_delta_final;
    case e_commodity::tobacco:
      return pr.tobacco_cigars.raw_delta_final;
    case e_commodity::cigars:
      return pr.tobacco_cigars.product_delta_final;
    case e_commodity::cotton:
      return pr.cotton_cloth.raw_delta_final;
    case e_commodity::cloth:
      return pr.cotton_cloth.product_delta_final;
    case e_commodity::fur: return pr.fur_coats.raw_delta_final;
    case e_commodity::coats:
      return pr.fur_coats.product_delta_final;
    case e_commodity::lumber:
      return pr.lumber_hammers.raw_delta_final;
    case e_commodity::silver: return pr.silver.raw_delta_final;
    case e_commodity::ore: return pr.ore_tools.raw_delta_final;
    case e_commodity::tools:
      return pr.tools_muskets.raw_delta_final;
    case e_commodity::muskets:
      return pr.tools_muskets.product_delta_final;
    case e_commodity::trade_goods: return pr.trade_goods;
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

// This will give the base colonist production for an indoor job
// that does not include expert status. The reason that we don't
// lump expert status together with this is because the original
// game tends to apply the expert bonus at a later stage in the
// chain of bonuses, potentially after some other bonuses are ap-
// plied (some of which may be additive, and some which may do
// rounding). For this reason, all units get the same production
// (3 in the original game) except for petty criminal, native
// convert, and indentured servant. If the colonist is an expert,
// that bonus will be applied separately.
//
// In the original game this seems to only depend on colonist
// type and not on the particular indoor job.
//
// Note that these numbers will be different from outdoor jobs.
// For example, a petty criminal is as good as a free colonist at
// farming.
int base_indoor_production_for_colonist_indoor_job(
    e_unit_type type ) {
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
    default: break;
  }
  return config_production.indoor_production
      .non_expert_base_production;
}

// Given n=5 and percent=50, this will yield 7.
void apply_int_percent_bonus_rnd_down( int& n, int percent ) {
  n = n + int( floor( n * double( percent ) / 100.0 ) );
}

// Given n=5 and percent=50, this will yield 8.
void apply_int_percent_bonus_rnd_up( int& n, int percent ) {
  n = n + int( ceil( n * double( percent ) / 100.0 ) );
}

[[nodiscard]] int apply_factory_reduction( int put,
                                           int factory_bonus ) {
  CHECK_GE( factory_bonus, 0 );
  // Avoid rounding errors in the common case.
  if( factory_bonus == 0 ) return put;
  // In the original game, factory_bonus is 50, so the below mul-
  // tiplies by 2/3 and rounds down.
  return int( floor( double( put ) * 100.0 /
                     ( 100.0 + factory_bonus ) ) );
}

struct BuildingBonusResult {
  int use = 0;
  int put = 0;
};

int bells_production( UnitsState const& units_state,
                      Player const& player, Colony const& colony,
                      BellsModifiers const& bells_modifiers ) {
  maybe<e_colony_building> maybe_building = building_for_slot(
      colony, e_colony_building_slot::town_hall );
  if( !maybe_building.has_value() ) {
    // If we have no relevant buildings then we're left with the
    // base value. Sanity check and leave.
    CHECK( colony.indoor_jobs[e_indoor_job::bells].size() == 0 );
    return 0;
  }

  e_colony_building const building = *maybe_building;

  // For base bells from the building itself, the original game
  // seems to do the following:
  //
  // 1. Add free building production (== 1).
  // 2. Add Paine bonus, multiply by (1+tax rate) (round down).
  //
  // Note the Jefferson bonus does not apply here. This is prob-
  // ably because the original game docs mention that his bonus
  // only increases the bell production of statesmen.
  //
  // Also, sons of liberty bonuses don't appear to apply to base
  // bell production. If they did, then a colony with one or two
  // colonists, once it gets to 100, could (depending on liberty
  // bell consumption) stay there comfortably with no statesmen.
  // Also, the base production is not produced by a colonist, and
  // sons of liberty bonuses (or tory penalties) are really sup-
  // posed to apply to colonists.
  //
  // Not sure if the original game applies the Paine bonus, but
  // it wouldn't really make a difference anyway, since even if
  // the tax rate is 99% then the bonus (which rounds down) would
  // yield nothing when applied to the base building production
  // of 1. So we're just including it here for consistency.
  //
  // The printing press/newspaper bonuses also apply here techni-
  // cally, but the original game appears to apply them at the
  // very end after the colonist contributions are added in.
  int buildings_quantity =
      config_production.free_building_production[building];
  if( player.fathers.has[e_founding_father::thomas_paine] )
    apply_int_percent_bonus_rnd_down(
        buildings_quantity, player.old_world.taxes.tax_rate );

  // For bells production from colonists, the original game seems
  // to do the following:
  //
  //   1. Non-expert colonist amount (1, 2, 3); for expert
  //      colonist use 3 (free colonist amount).
  //   2. Add sons of liberty bonus/penalty (+1/+2).
  //   3. Subtract tory penalty (-1)*tory_population/N, where N
  //      is determined by difficulty level.
  //   4. Multiply by two for expert.
  //   5. Add Jefferson bonus, multiply by 1.5 (rounding up).
  //   6. Add Paine bonus, multiply by (1+tax rate) (round down).

  vector<UnitId> const& unit_ids =
      colony.indoor_jobs[e_indoor_job::bells];
  int        units_quantity = 0;
  bool const has_jefferson =
      player.fathers.has[e_founding_father::thomas_jefferson];
  bool const has_paine =
      player.fathers.has[e_founding_father::thomas_paine];
  for( UnitId id : unit_ids ) {
    e_unit_type const unit_type =
        units_state.unit_for( id ).type();
    int unit_quantity =
        base_indoor_production_for_colonist_indoor_job(
            unit_type );
    bells_modifiers.apply_indoor( unit_quantity );
    if( indoor_unit_is_expert( e_indoor_job::bells, unit_type ) )
      apply_int_percent_bonus_rnd_down(
          unit_quantity,
          config_production.indoor_production.expert_bonus );
    if( has_jefferson )
      apply_int_percent_bonus_rnd_up(
          unit_quantity, config_production.indoor_production
                             .thomas_jefferson_bells_bonus );
    if( has_paine )
      apply_int_percent_bonus_rnd_down(
          unit_quantity, player.old_world.taxes.tax_rate );

    units_quantity += unit_quantity;
  }

  int const pre_newspaper_total =
      buildings_quantity + units_quantity;

  // Lastly, the original game will apply the printing press/new-
  // paper bonuses after all others are added. This means:
  //
  //   - Multiply by 1.5 for printing press (rounding down) or 2
  //     for newspaper.
  //
  // To emphasize, this multiplicative bonus applies to both the
  // unit quantities and the town hall quantity.
  int const post_newspaper_total = [&] {
    maybe<e_colony_building> bells_bonus_building =
        building_for_slot( colony,
                           e_colony_building_slot::newspapers );
    int res = pre_newspaper_total;
    apply_int_percent_bonus_rnd_down(
        res, bells_bonus_building.has_value()
                 ? config_production.building_production_bonus
                       [*bells_bonus_building]
                 : 0 );
    return res;
  }();

  int const total = post_newspaper_total;

  return total;
}

int crosses_production_for_colony(
    UnitsState const& units_state, Player const& player,
    Colony const&         colony,
    BellsModifiers const& bells_modifiers ) {
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
  e_colony_building const building = *maybe_building;

  int const buildings_quantity =
      config_production.free_building_production[building];

  // In the original game, base crosses don't seem to have any
  // bonuses applied, even together with colonists.

  // The original game seems to compute colonist contributions in
  // this way:
  //
  //   1. Non-expert colonist amount (1, 2, 3); for expert
  //      colonist use 3 (free colonist amount).
  //   2. Multiply by two for expert.
  //   3. Add sons of liberty bonus/penalty (+2).
  //   4. Subtract tory penalty (-1)*tory_population/N, where N
  //      is determined by difficulty level.
  //   5. Multiply by two for building bonus.
  //   6. Add William Penn bonus, multiply by 1.5 (round down).
  //
  // The original game appears to apply the William Penn bonus on
  // a per-colonist basis, and rounds down.
  vector<UnitId> const& unit_ids =
      colony.indoor_jobs[e_indoor_job::crosses];
  int        units_quantity = 0;
  bool const has_penn =
      player.fathers.has[e_founding_father::william_penn];
  for( UnitId id : unit_ids ) {
    int               unit_quantity = 0;
    e_unit_type const unit_type =
        units_state.unit_for( id ).type();
    unit_quantity =
        base_indoor_production_for_colonist_indoor_job(
            unit_type );
    if( indoor_unit_is_expert( e_indoor_job::crosses,
                               unit_type ) )
      apply_int_percent_bonus_rnd_down(
          unit_quantity,
          config_production.indoor_production.expert_bonus );
    bells_modifiers.apply_indoor( unit_quantity );
    apply_int_percent_bonus_rnd_down(
        unit_quantity,
        config_production.building_production_bonus[building] );
    if( has_penn )
      apply_int_percent_bonus_rnd_down(
          unit_quantity, config_production.indoor_production
                             .william_penn_crosses_bonus );
    units_quantity += unit_quantity;
  }

  int total =
      base_quantity + buildings_quantity + units_quantity;

  return total;
}

void compute_food_production(
    TerrainState const& terrain_state,
    UnitsState const& units_state, Colony const& colony,
    BellsModifiers const& bells_modifiers,
    int const center_food_produced, FoodProduction& out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( maybe<OutdoorUnit> const& unit = colony.outdoor_jobs[d];
        unit.has_value() && unit->job == e_outdoor_job::food ) {
      int quantity = production_on_square(
          e_outdoor_job::food, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location.moved( d ) );
      bells_modifiers.apply_outdoor( quantity );
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
      int quantity = production_on_square(
          e_outdoor_job::fish, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location.moved( d ) );
      bells_modifiers.apply_outdoor( quantity );
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

  int const current_food = colony.commodities[e_commodity::food];
  out.food_consumed_by_colonists_actual =
      ( current_food + out.food_produced ) >=
              out.food_consumed_by_colonists_theoretical
          ? out.food_consumed_by_colonists_theoretical
          : ( current_food + out.food_produced );
  CHECK_GE( out.food_consumed_by_colonists_actual, 0 );

  // Either zero or positive.
  out.food_deficit =
      ( current_food + out.food_produced ) >=
              out.food_consumed_by_colonists_theoretical
          ? 0
          : out.food_consumed_by_colonists_theoretical -
                ( current_food + out.food_produced );
  CHECK_GE( out.food_deficit, 0 );

  out.food_surplus_before_horses = std::max(
      0, out.food_produced -
             out.food_consumed_by_colonists_theoretical );

  // We must have at least two horses to breed. If we do, then we
  // produce two extra horses per 50 (or less) horses. I.e., 50
  // horses produces two horses per turn, and 51 produces four
  // per turn.
  int const current_horses =
      colony.commodities[e_commodity::horses];
  out.horses_produced_theoretical =
      ( current_horses < 2 )
          ? 0
          : 2 * ( ( current_horses + 49 ) / 50 );
  if( colony.buildings[e_colony_building::stable] )
    out.horses_produced_theoretical *= 2;
  // In the original game, horses are allowed to consume at most
  // half of the surplus food, rounded up.
  out.max_horse_food_consumption_allowed = static_cast<int>(
      ceil( out.food_surplus_before_horses / 2.0 ) );
  int const food_per_new_horse = 1;
  out.max_new_horses_allowed =
      out.max_horse_food_consumption_allowed /
      food_per_new_horse;

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

  // Do this again since it is important.
  CHECK_GE( out.food_deficit, 0 );

  if( out.food_deficit > 0 ) {
    CHECK( out.horses_produced_actual == 0 );
    // Final food delta can be computed without regard to horses.
    int const proposed_new_food_quantity = 0;
    // Since there are no warehouse limits on the amount of food,
    // we can just compute the final delta.
    out.food_delta_final =
        ( proposed_new_food_quantity - current_food );
    CHECK( current_food + out.food_delta_final == 0 );
    out.colonist_starved = true;
  } else {
    // Final food delta must take into account horses.
    int const proposed_new_food_quantity =
        current_food + out.food_produced -
        out.food_consumed_by_colonists_actual -
        out.food_consumed_by_horses;
    out.food_delta_final =
        proposed_new_food_quantity - current_food;
    // Note that food_delta_final could be positive or negative
    // here, since the fact that we have no food deficit may just
    // mean that there was enough in the warehouse to draw from.
    CHECK( out.colonist_starved == false );
  }

  CHECK( out.food_delta_final + current_food >= 0,
         "colony supply of food has gone negative ({}).",
         out.food_delta_final + current_food );
  if( out.colonist_starved ) {
    CHECK( out.food_delta_final + current_food == 0 );
  }
}

void compute_raw(
    Colony const& colony, TerrainState const& terrain_state,
    UnitsState const& units_state, e_outdoor_job outdoor_job,
    maybe<SquareProduction const&> center_secondary,
    BellsModifiers const&          bells_modifiers,
    RawMaterialAndProduct&         out,
    refl::enum_map<e_direction, SquareProduction>&
        out_land_production ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( maybe<OutdoorUnit> const& unit = colony.outdoor_jobs[d];
        unit.has_value() && unit->job == outdoor_job ) {
      int quantity = production_on_square(
          outdoor_job, terrain_state,
          units_state.unit_for( unit->unit_id ).type(),
          colony.location.moved( d ) );
      bells_modifiers.apply_outdoor( quantity );
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
                      BellsModifiers const&  bells_modifiers,
                      RawMaterialAndProduct& out ) {
  e_colony_building_slot const building_slot =
      slot_for_indoor_job( indoor_job );
  maybe<e_colony_building> const building =
      building_for_slot( colony, building_slot );
  if( !building.has_value() ) {
    CHECK( colony.indoor_jobs[indoor_job].empty() );
    return;
  }

  // For each unit, the original game appears to apply bonuses in
  // the following way to arrive at the "put" quantity per
  // colonist, which is the amount produced per colonist. In gen-
  // eral this differs from the amount of raw material consumed
  // due to factory level buildings. The original game appears to
  // compute the "put" first, sum it over all colonists, then de-
  // rive the consumed quantity from it.
  //
  //   1. Non-expert colonist amount (1, 2, 3); for expert
  //      colonist use 3 (free colonist amount).
  //   2. Multiply by 2 for building upgrade.
  //   3. Add sons of liberty bonus (+1/+2).
  //   4. Subtract tory penalty (-1)*tory_population/N, where N
  //      is determined by difficulty level.
  //   5. Multiply by 1.5 for factory level (rounding down).
  //   6. Multiply by 2 for expert.
  //
  int units_quantity_put = 0;
  for( UnitId unit_id : colony.indoor_jobs[indoor_job] ) {
    int               unit_quantity_put = 0;
    e_unit_type const unit_type =
        units_state.unit_for( unit_id ).type();
    unit_quantity_put +=
        base_indoor_production_for_colonist_indoor_job(
            unit_type );
    apply_int_percent_bonus_rnd_down(
        unit_quantity_put,
        config_production.building_production_bonus[*building] );
    bells_modifiers.apply_indoor( unit_quantity_put );
    // Note the factory bonus may be zero.
    apply_int_percent_bonus_rnd_down(
        unit_quantity_put,
        config_production.factory_bonus[*building] );
    if( indoor_unit_is_expert( indoor_job, unit_type ) )
      apply_int_percent_bonus_rnd_down(
          unit_quantity_put,
          config_production.indoor_production.expert_bonus );

    units_quantity_put += unit_quantity_put;
  }

  out.product_produced_theoretical += units_quantity_put;

  // The "put" quantity has already been inflated, so now we de-
  // flate it to get the consumption quantity (this seems to be
  // how the original game does it, and it matters because of the
  // fact that it is being applied to the sum of all colonists'
  // production and the rounding behavior).
  int const consumed_theoretical = apply_factory_reduction(
      units_quantity_put,
      config_production.factory_bonus[*building] );

  out.raw_consumed_theoretical = consumed_theoretical;
  out.raw_delta_theoretical -= out.raw_consumed_theoretical;

  // The quantities actually produced and consumed might have to
  // be lowered from their theoretical values if there isn't
  // enough total supply of the raw material. But because the
  // building might be factory-level we need to then recompute
  // what is actually produced using the factory bonus.
  //
  // Note that the colony commodities at this point will already
  // include the raw product this turn.
  int const available_raw_input =
      colony.commodities[raw_commodity];
  out.raw_consumed_actual = std::min(
      out.raw_consumed_theoretical, available_raw_input );
  // Theoretically there is an ambiguity here as to whether we
  // should round up or down. In the above, when we apply the
  // factory bonus to derive the "put" value, we round down (be-
  // cause the original game seems to do that). That means that
  // there are multiple values that could lead to a certain "con-
  // sumed" value. The original game appears to round up here, so
  // we will do that.
  out.product_produced_actual = out.raw_consumed_actual;
  apply_int_percent_bonus_rnd_up(
      out.product_produced_actual,
      config_production.factory_bonus[*building] );
}

void compute_land_production(
    ColonyProduction& pr, Colony const& colony_pristine,
    TerrainState const&   terrain_state,
    UnitsState const&     units_state,
    BellsModifiers const& bells_modifiers ) {
  // FIXME: copying not optimal.
  Colony colony = colony_pristine;

  auto do_product =
      [&]( e_indoor_job job, e_commodity raw,
           RawMaterialAndProduct& raw_and_product ) {
        compute_product( colony, job, units_state, raw,
                         bells_modifiers, raw_and_product );
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
                 pr.center_extra_production, bells_modifiers,
                 raw_and_product, pr.land_production );
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
  compute( e_outdoor_job::silver, pr.silver );

  // In the original game it appears that hammer production
  // bonuses are applied in a different order than for the other
  // derived goods. Specifically, it seems to go like this:
  //
  //   1. Non-expert colonist production (1, 2, 3).
  //   2. Apply expert bonus (x2).
  //   3. Add sons of liberty bonus/penalty.
  //   4. Multiply by 2 for building upgrade.
  //
  // (There is no factory bonus step because in the original game
  // there is no factory-level carpentry building). Hence, the
  // numbers turn out a bit differently than for the other
  // building workers, such as e.g. a rum distiller. In particu-
  // lar, this causes an expert to no longer produce exactly
  // twice the amount of a free colonist when there is a SoL
  // bonus/penalty. Instead of making an exception and computing
  // hammers differently, we will just compute them in the same
  // way as the other derived goods. This shouldn't be a problem
  // because in the common case, where there is no SoL
  // bonus/penalty, there is no difference; when there is a dif-
  // ference, it is not so severe. And this way it will be easier
  // for the player to understand since they don't have to learn
  // a separate rule for hammer production.
  compute( e_outdoor_job::lumber, pr.lumber_hammers );

  // Even though the product for this one (tools) are then con-
  // sumed by a gunsmith, the original game seems to just compute
  // these first as a normal raw/product pair as if muskets did
  // not exist. Then computes musket production in the next step.
  compute( e_outdoor_job::ore, pr.ore_tools );

  // Tools/Muskets. Boostrap the muskets calculation with the
  // tools produced from the ore/tools stage. Note that the
  // raw_produced (tools) here is the value before warehouse cut-
  // offs are considered, since anything produced this turn be-
  // yond that is allowed to be consumed within the same turn.
  // Spoilage only applies after all of the dust settles.
  pr.tools_muskets.raw_produced =
      pr.ore_tools.product_produced_actual;
  pr.tools_muskets.raw_delta_theoretical =
      pr.tools_muskets.raw_produced;
  do_product( e_indoor_job::muskets, e_commodity::tools,
              pr.tools_muskets );
  colony.commodities[e_commodity::tools] -=
      pr.tools_muskets.raw_consumed_actual;

  compute_food_production( terrain_state, units_state, colony,
                           bells_modifiers,
                           pr.center_food_production,
                           pr.food_horses, pr.land_production );
  colony.commodities[e_commodity::horses] +=
      pr.food_horses.horses_produced_actual;

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

  for( e_commodity c : refl::enum_values<e_commodity> )
    if( config_colony.warehouses
            .commodities_with_warehouse_limit[c] )
      adjust_for_warehouse(
          c, final_production_delta_for_commodity( pr, c ) );

  // We need to do hammers manually since it won't be done by the
  // loop above because there is no commodity for hammers. Since
  // there is no limit on the number of hammers that can be accu-
  // mulated, the final and actual are the same.
  pr.lumber_hammers.product_delta_final =
      pr.lumber_hammers.product_produced_actual;

  // It's nice for these to agree, even though the policy is to
  // only read from the tools_muskets version.
  pr.ore_tools.product_delta_final =
      pr.tools_muskets.raw_delta_final;
}

void fill_in_center_square(
    SSConst const& ss, Colony const& colony,
    BellsModifiers const& bells_modifiers,
    ColonyProduction&     pr ) {
  MapSquare const& square =
      ss.terrain.square_at( colony.location );

  // Food.
  pr.center_food_production = food_production_on_center_square(
      square, ss.settings.difficulty );
  bells_modifiers.apply_outdoor( pr.center_food_production );

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
  bells_modifiers.apply_outdoor(
      pr.center_extra_production->quantity );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
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

int const& final_production_delta_for_commodity(
    ColonyProduction const& pr, e_commodity c ) {
  switch( c ) {
    case e_commodity::food:
      return pr.food_horses.food_delta_final;
    case e_commodity::horses:
      return pr.food_horses.horses_delta_final;
    case e_commodity::sugar: return pr.sugar_rum.raw_delta_final;
    case e_commodity::rum:
      return pr.sugar_rum.product_delta_final;
    case e_commodity::tobacco:
      return pr.tobacco_cigars.raw_delta_final;
    case e_commodity::cigars:
      return pr.tobacco_cigars.product_delta_final;
    case e_commodity::cotton:
      return pr.cotton_cloth.raw_delta_final;
    case e_commodity::cloth:
      return pr.cotton_cloth.product_delta_final;
    case e_commodity::fur: return pr.fur_coats.raw_delta_final;
    case e_commodity::coats:
      return pr.fur_coats.product_delta_final;
    case e_commodity::lumber:
      return pr.lumber_hammers.raw_delta_final;
    case e_commodity::silver: return pr.silver.raw_delta_final;
    case e_commodity::ore: return pr.ore_tools.raw_delta_final;
    case e_commodity::tools:
      // For tools, its the one in tools/muskets that has the
      // final value.
      return pr.tools_muskets.raw_delta_final;
    case e_commodity::muskets:
      return pr.tools_muskets.product_delta_final;
    case e_commodity::trade_goods: return pr.trade_goods;
  }
}

maybe<int> production_for_slot( ColonyProduction const& pr,
                                e_colony_building_slot  slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return pr.tools_muskets.product_produced_theoretical;
    case e_colony_building_slot::tools:
      // For tools, its the one in tools/muskets that has the
      // final value.
      return pr.tools_muskets.raw_produced;
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
      return pr.food_horses.horses_produced_theoretical;
    case e_colony_building_slot::wall: return nothing;
    case e_colony_building_slot::warehouses: return nothing;
    case e_colony_building_slot::crosses: return pr.crosses;
    case e_colony_building_slot::custom_house: return nothing;
  }
}

BellsModifiers compute_bells_modifiers(
    Player const& player, Colony const& colony,
    e_difficulty difficulty ) {
  int const population = colony_population( colony );

  // This won't happen in practice because the game does not
  // allow zero-population colonies, but it is useful for unit
  // tests where that something happens, and which would other-
  // wise cause code below to check-fail. Conceptually it makes
  // sense either way, because a population zero colony cannot
  // have any sons of liberty, and thus no sons of liberty bonus,
  // and also has no tories, so cannot have a tory penalty.
  if( population == 0 ) return BellsModifiers{};

  double const sons_of_liberty_percent =
      compute_sons_of_liberty_percent(
          colony.sons_of_liberty.num_rebels_from_bells_only,
          population,
          player.fathers.has[e_founding_father::simon_bolivar] );

  int const sons_of_liberty_integral_percent =
      compute_sons_of_liberty_integral_percent(
          sons_of_liberty_percent );

  int const sons_of_liberty_number =
      compute_sons_of_liberty_number(
          sons_of_liberty_integral_percent, population );

  int const tory_number =
      compute_tory_number( sons_of_liberty_number, population );

  int const tory_penalty_outdoor =
      compute_tory_penalty_outdoor( difficulty, tory_number );
  int const tory_penalty_indoor =
      compute_tory_penalty_indoor( difficulty, tory_number );

  int const sons_of_liberty_bonus_outdoor =
      compute_sons_of_liberty_bonus_outdoor(
          sons_of_liberty_integral_percent );
  int const sons_of_liberty_bonus_indoor =
      compute_sons_of_liberty_bonus_indoor(
          sons_of_liberty_integral_percent );

  return BellsModifiers{
      .sons_of_liberty_bonus_outdoor =
          sons_of_liberty_bonus_outdoor,
      .sons_of_liberty_bonus_indoor =
          sons_of_liberty_bonus_indoor,
      .tory_penalty_outdoor = tory_penalty_outdoor,
      .tory_penalty_indoor  = tory_penalty_indoor };
}

ColonyProduction production_for_colony( SSConst const& ss,
                                        Colony const&  colony ) {
  ColonyProduction res;
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );

  // These are computed based on the state of things last turn.
  BellsModifiers const bells_modifiers = compute_bells_modifiers(
      player, colony, ss.settings.difficulty );

  res.crosses = crosses_production_for_colony(
      ss.units, player, colony, bells_modifiers );

  res.bells = bells_production( ss.units, player, colony,
                                bells_modifiers );

  // Note that this must be done before processing any of the
  // other land squares.
  fill_in_center_square( ss, colony, bells_modifiers, res );

  compute_land_production( res, colony, ss.terrain, ss.units,
                           bells_modifiers );

  CHECK( res.trade_goods == 0 );
  return res;
}

} // namespace rn
