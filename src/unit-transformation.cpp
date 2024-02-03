/****************************************************************
**unit-transformation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-04.
*
* Description: Represents unit type and inventory, along with
*              type transformations resulting from inventory
*              changes.
*
*****************************************************************/
#include "unit-transformation.hpp"

// Revolution Now
#include "unit-mgr.hpp"

// config
#include "config/unit-type.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/to-str-ext-std.hpp"
#include "base/to-str-tags.hpp"

using namespace std;

namespace rn {

using ::base::FmtVerticalMap;

namespace {

// Commodity quantity can be positive or negative.
vector<UnitTransformationFromCommodity> unit_delta_commodity(
    UnitComposition const& comp, Commodity const& commodity ) {
  vector<UnitTransformation> general_results =
      possible_unit_transformations(
          comp, { { commodity.type, commodity.quantity } } );
  vector<UnitTransformationFromCommodity> res;
  res.reserve( general_results.size() );
  for( UnitTransformation const& utr : general_results ) {
    if( utr.commodity_deltas.size() != 1 ) continue;
    if( utr.modifier_deltas.size() > 1 ) continue;
    DCHECK( utr.commodity_deltas.size() == 1 );
    // quantity_delta is positive if the unit takes some.
    auto [comm, quantity_delta] = *utr.commodity_deltas.begin();
    DCHECK( quantity_delta != 0 );
    if( comm != commodity.type )
      // I think this could happen if a transformation entails
      // the unit shedding a single commodity which happens to be
      // different from the one in question here.
      continue;
    // At this point we know that the transformation requires
    // changing only a single commodity, and that commodity is
    // the one in question here. Now make sure that the quantity
    // change is in the direction that we're asking.
    if( ( quantity_delta > 0 ) == ( commodity.quantity > 0 ) )
      continue;
    // Make sure that the abs quantity added/removed is less than
    // the abs quantity requested. In other words, if we ask to
    // take 10, we could take 5, but not 20. If we give ten, then
    // the unit could accept 5, but not 20. This is mainly for
    // pioneers that can accept (or yield) a variable amount of
    // tools but whose absolute value is limited.
    if( abs( quantity_delta ) > abs( commodity.quantity ) )
      continue;
    res.push_back( UnitTransformationFromCommodity{
        .new_comp        = utr.new_comp,
        .modifier_deltas = utr.modifier_deltas,
        .quantity_used   = -quantity_delta } );
  }
  return res;
}

} // namespace

/****************************************************************
** Commodity Conversion
*****************************************************************/
namespace {

maybe<Commodity> commodity_from_modifier(
    UnitComposition const& comp, e_unit_type_modifier mod ) {
  UnitTypeModifierTraits const& traits =
      config_unit_type.composition.modifier_traits[mod];
  switch( traits.association.to_enum() ) {
    using e = ModifierAssociation::e;
    case e::none:
      return nothing;
    case e::commodity: {
      auto const& o = traits.association
                          .get<ModifierAssociation::commodity>();
      return o.commodity;
    }
    case e::inventory: {
      auto const& o = traits.association
                          .get<ModifierAssociation::inventory>();
      int const quantity = comp.inventory()[o.type];
      UnitInventoryTraits const& inv_traits =
          config_unit_type.composition.inventory_traits[o.type];
      UNWRAP_RETURN( comm_type, inv_traits.commodity );
      return Commodity{ .type     = comm_type,
                        .quantity = quantity };
    }
  }
}

unordered_map<e_commodity, int> commodities_in_unit(
    UnitComposition const& comp ) {
  unordered_map<e_commodity, int>            res;
  unordered_set<e_unit_type_modifier> const& mods =
      comp.type_obj().unit_type_modifiers();
  for( e_unit_type_modifier mod : mods ) {
    maybe<Commodity> comm = commodity_from_modifier( comp, mod );
    if( !comm.has_value() ) continue;
    DCHECK( comm->quantity > 0 );
    res[comm->type] += comm->quantity;
  }
  // Now do inventory types that are not associated with a modi-
  // fier.
  for( auto [inv, q] : comp.inventory() ) {
    if( inventory_to_modifier( inv ).has_value() ) continue;
    // Inventory type has no associated modifier.
    maybe<e_commodity> comm = inventory_to_commodity( inv );
    if( !comm.has_value() ) continue;
    // Inventory type is associated with a commodity.
    res[*comm] += q;
  }
  return res;
}

maybe<int> max_valid_inventory_quantity(
    ModifierAssociation::inventory const& inventory,
    int                                   max_available ) {
  UnitInventoryTraits const& inv_traits =
      config_unit_type.composition
          .inventory_traits[inventory.type];
  int adjusted_max =
      std::min( max_available, inv_traits.max_quantity );
  adjusted_max -= adjusted_max % inv_traits.multiple;
  if( adjusted_max < inv_traits.min_quantity ) return nothing;
  return adjusted_max;
}

void remove_commodities_from_inventory(
    UnitComposition::UnitInventoryMap& inventory ) {
  for( auto inv : refl::enum_values<e_unit_inventory> )
    if( inventory_to_commodity( inv ) ) inventory[inv] = 0;
}

} // namespace

/****************************************************************
** Transformations
*****************************************************************/
vector<UnitTransformation> possible_unit_transformations(
    UnitComposition const&                 comp,
    unordered_map<e_commodity, int> const& commodity_store ) {
  // 1. If current unit is a derived type, convert all of its
  // modifiers and inventory to commodities where possible.
  unordered_map<e_commodity, int> const starting_comms = [&] {
    unordered_map<e_commodity, int> res = commodity_store;
    for( auto [type, q] : commodities_in_unit( comp ) )
      res[type] += q;
    return res;
  }();
  // 2. Iterate through list of derived types and record com-
  // modity delta and modifier delta and add item to result.
  auto const& old_mods = comp.type_obj().unit_type_modifiers();
  unordered_map<e_unit_type, unordered_set<e_unit_type_modifier>>
      mods_map = unit_attr( comp.base_type() ).modifiers;
  // Add in base type because we want that to be among the
  // choices. E.g., if the unit starts out as a soldier, then the
  // free_colonist should be among the results.
  DCHECK( !mods_map.contains( comp.base_type() ) );
  mods_map[comp.base_type()] = {};
  vector<UnitTransformation> res;
  for( auto const& [new_type, mods] : mods_map ) {
    unordered_map<e_commodity, int> commodities = starting_comms;
    // First subtract commodities required by modifiers that re-
    // quire a fixed number of commodities.
    for( e_unit_type_modifier mod : mods ) {
      maybe<Commodity> comm =
          config_unit_type.composition.modifier_traits[mod]
              .association
              .get_if<ModifierAssociation::commodity>()
              .member(
                  &ModifierAssociation::commodity::commodity );
      if( !comm.has_value() ) continue;
      DCHECK( comm->quantity > 0 );
      commodities[comm->type] -= comm->quantity;
    }
    // Next determine commodities required in inventory.
    refl::enum_map<e_unit_inventory, int> new_inventory =
        comp.inventory();
    // We remove these because we've already added them into the
    // starting_comms above.
    remove_commodities_from_inventory( new_inventory );
    for( e_unit_type_modifier mod : mods ) {
      auto inventory =
          config_unit_type.composition.modifier_traits[mod]
              .association
              .get_if<ModifierAssociation::inventory>();
      if( !inventory.has_value() )
        // Does not require any inventory commodities.
        continue;
      maybe<e_commodity> comm =
          config_unit_type.composition
              .inventory_traits[inventory->type]
              .commodity;
      if( !comm.has_value() ) continue;
      maybe<int> quantity_to_use = max_valid_inventory_quantity(
          *inventory,
          /*max_available=*/commodities[*comm] );
      if( !quantity_to_use.has_value() )
        // A modifier that requires inventory commodities cannot
        // be satisfied due to not enough of that commodity in
        // supply, so we skip to the next unit type.
        goto next_unit_type;
      commodities[*comm] -= *quantity_to_use;
      new_inventory[inventory->type] += *quantity_to_use;
    }
    // Finally check if we haven't run out of commodities.
    for( auto [type, q] : commodities ) {
      if( q < 0 )
        // This transformation cannot be satisfied since we don't
        // have enough of a particular commodity.
        goto next_unit_type;
    }
    {
      // We're good to go.
      UNWRAP_CHECK(
          new_unit_type_obj,
          UnitType::create( new_type, comp.base_type() ) );
      UNWRAP_CHECK( new_comp,
                    UnitComposition::create( new_unit_type_obj,
                                             new_inventory ) );
      unordered_map<e_unit_type_modifier,
                    e_unit_type_modifier_delta>
                                                 modifier_deltas;
      unordered_set<e_unit_type_modifier> const& new_mods =
          new_unit_type_obj.unit_type_modifiers();
      for( auto mod : refl::enum_values<e_unit_type_modifier> ) {
        if( old_mods.contains( mod ) &&
            !new_mods.contains( mod ) )
          modifier_deltas[mod] = e_unit_type_modifier_delta::del;
        else if( !old_mods.contains( mod ) &&
                 new_mods.contains( mod ) )
          modifier_deltas[mod] = e_unit_type_modifier_delta::add;
      }
      unordered_map<e_commodity, int> commodity_deltas;
      for( auto comm_type : refl::enum_values<e_commodity> ) {
        int orig_quantity =
            base::lookup( commodity_store, comm_type )
                .value_or( 0 );
        int new_quantity = commodities[comm_type];
        int delta        = new_quantity - orig_quantity;
        if( delta == 0 ) continue;
        commodity_deltas[comm_type] = delta;
      }

      res.push_back( UnitTransformation{
          .new_comp         = std::move( new_comp ),
          .modifier_deltas  = std::move( modifier_deltas ),
          .commodity_deltas = std::move( commodity_deltas ) } );
    }
    // !! This label must be last line of loop.
  next_unit_type:
    continue;
  }
  return res;
}

UnitTransformation strip_to_base_type(
    UnitComposition const& comp ) {
  vector<UnitTransformation> general_results =
      possible_unit_transformations( comp,
                                     /*commodity_store=*/{} );
  erase_if( general_results,
            L( _.new_comp.type() != _.new_comp.base_type() ) );
  CHECK( general_results.size() == 1,
         "unit {} does not have a unique base type: {}", comp,
         general_results );
  auto& res = general_results[0];
  for( auto [comm_type, q] : res.commodity_deltas ) {
    CHECK( q > 0,
           "while stripping unit of commodities, a negative "
           "commodity was returned ({}, {}).",
           comm_type, q );
  }
  return std::move( general_results[0] );
}

vector<UnitTransformationFromCommodity> unit_receive_commodity(
    UnitComposition const& comp, Commodity const& commodity ) {
  CHECK_GT( commodity.quantity, 0 );
  return unit_delta_commodity( comp, commodity );
}

std::vector<UnitTransformationFromCommodity> unit_lose_commodity(
    UnitComposition const& comp, Commodity const& commodity ) {
  CHECK_GT( commodity.quantity, 0 );
  Commodity new_comm{ .type     = commodity.type,
                      .quantity = -commodity.quantity };
  return unit_delta_commodity( comp, new_comm );
}

vector<UnitTransformationFromCommodity> with_commodity_added(
    Unit const& unit, Commodity const& commodity ) {
  return unit_receive_commodity( unit.composition(), commodity );
}

vector<UnitTransformationFromCommodity> with_commodity_removed(
    Unit const& unit, Commodity const& commodity ) {
  return unit_lose_commodity( unit.composition(), commodity );
}

void consume_20_tools( SS& ss, TS& ts, Unit& unit ) {
  vector<UnitTransformationFromCommodity> results =
      with_commodity_removed(
          unit, Commodity{ .type     = e_commodity::tools,
                           .quantity = 20 } );
  vector<UnitTransformationFromCommodity> valid_results;
  for( auto const& result : results ) {
    // It would be e.g. -80 because one valid transformation is
    // that we could subtract more than 20 tools, give the
    // blessing mod, and turn the unit into a missionary. But we
    // are not looking for that here.
    if( result.quantity_used != -20 ) continue;
    if( result.modifier_deltas.empty() ||
        result.modifier_deltas ==
            unordered_map<e_unit_type_modifier,
                          e_unit_type_modifier_delta>{
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } } ) {
      valid_results.push_back( result );
    }
  }
  CHECK( valid_results.size() == 1,
         "could not find viable target unit after tools "
         "removed. results: {}",
         results );
  // This won't always change the type; e.g. it might just re-
  // place the type with the same type but with fewer tools in
  // the inventory.
  change_unit_type( ss, ts, unit, valid_results[0].new_comp );
}

namespace {

template<typename T>
bool requires_independence( T const& utr ) {
  for( auto [modifier, delta] : utr.modifier_deltas )
    if( modifier == e_unit_type_modifier::independence &&
        delta == e_unit_type_modifier_delta::add )
      return true;
  return false;
}

} // namespace

void adjust_for_independence_status(
    vector<UnitTransformation>& input,
    bool                        independence_declared ) {
  if( independence_declared ) return;
  erase_if( input, L( requires_independence( _ ) ) );
}

void adjust_for_independence_status(
    vector<UnitTransformationFromCommodity>& input,
    bool independence_declared ) {
  if( independence_declared ) return;
  erase_if( input, L( requires_independence( _ ) ) );
}

} // namespace rn
