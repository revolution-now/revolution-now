/****************************************************************
**unit-composer.cpp
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
#include "unit-composer.hpp"

// Revolution Now
#include "config-files.hpp"
#include "lua.hpp"

// Revolution Now (config)
#include "../config/rcl/units.inl"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"

using namespace std;

namespace rn {

/****************************************************************
** UnitComposition
*****************************************************************/
UnitComposition::UnitComposition( UnitType         type,
                                  UnitInventoryMap inventory )
  : type_( type ), inventory_( std::move( inventory ) ) {}

valid_deserial_t UnitComposition::check_invariants_safe() const {
  // Validation: make sure that quantities of inventory items are
  // within range.
  for( e_unit_inventory type :
       enum_values<e_unit_inventory>() ) {
    maybe<int const&> quantity =
        base::lookup( inventory_, type );
    if( !quantity.has_value() ) continue;
    UnitInventoryTraits const& traits =
        config_units.composition.inventory_traits[type];
    VERIFY_DESERIAL(
        *quantity >= traits.min_quantity,
        fmt::format( "{} inventory must have at least {} items.",
                     type, traits.min_quantity ) );
    VERIFY_DESERIAL(
        *quantity <= traits.max_quantity,
        fmt::format( "{} inventory must have at most {} items.",
                     type, traits.max_quantity ) );
    VERIFY_DESERIAL(
        *quantity % traits.multiple == 0,
        fmt::format(
            "{} inventory must come in multiples of {} items.",
            type, traits.multiple ) );
  }

  // FIXME
  return base::valid;
}

UnitComposition UnitComposition::create( UnitType type ) {
  // TODO: need to add default inventory for unit type...
  UNWRAP_CHECK(
      res, UnitComposition::create( type, /*inventory=*/{} ) );
  return res;
}

UnitComposition UnitComposition::create( e_unit_type type ) {
  return UnitComposition::create( UnitType::create( type ) );
}

maybe<UnitComposition> UnitComposition::create(
    UnitType type, UnitInventoryMap const& inventory ) {
  auto res = UnitComposition( type, inventory );
  if( !res.check_invariants_safe() ) return nothing;
  return res;
}

maybe<UnitComposition> UnitComposition::with_new_type(
    UnitType type ) const {
  return create( type, inventory_ );
}

void to_str( UnitComposition const& o, string& out ) {
  out += fmt::format( "UnitComposition{{type={},inventory={}}}",
                      o.type_, o.inventory_ );
  // out += "UnitComposition";
  // out += fmt::format(
  //     "{}",
  //     FmtVerticalMap{ unordered_map<string, string>{
  //         { "type", fmt::to_string( o.type_ ) },
  //         { "inventory",
  //           fmt::to_string( FmtVerticalMap{ o.inventory_ } )
  //           },
  //     } } );
}

/****************************************************************
** Commodity Conversion
*****************************************************************/
namespace {

maybe<Commodity> commodity_from_modifier(
    UnitComposition const& comp, e_unit_type_modifier mod ) {
  UnitTypeModifierTraits const& traits =
      config_units.composition.modifier_traits[mod];
  switch( traits.association.to_enum() ) {
    using namespace ModifierAssociation;
    case e::none: return nothing;
    case e::commodity: {
      auto const& o = traits.association.get<commodity>();
      return o.commodity;
    }
    case e::inventory: {
      auto const& o = traits.association.get<inventory>();
      UNWRAP_RETURN( quantity,
                     base::lookup( comp.inventory(), o.type ) );
      UnitInventoryTraits const& inv_traits =
          config_units.composition.inventory_traits[o.type];
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
    // This includes inventory as well, since we assume that any
    // inventory item that represents a commodity must have an
    // associated modifier.
    maybe<Commodity> comm = commodity_from_modifier( comp, mod );
    if( !comm.has_value() ) continue;
    DCHECK( comm->quantity > 0 );
    res[comm->type] += comm->quantity;
  }
  return res;
}

maybe<int> max_valid_inventory_quantity(
    ModifierAssociation::inventory const& inventory,
    int                                   max_available ) {
  UnitInventoryTraits const& inv_traits =
      config_units.composition.inventory_traits[inventory.type];
  int adjusted_max =
      std::min( max_available, inv_traits.max_quantity );
  adjusted_max -= adjusted_max % inv_traits.multiple;
  if( adjusted_max < inv_traits.min_quantity ) return nothing;
  return adjusted_max;
}

void remove_commodities_from_inventory(
    UnitComposition::UnitInventoryMap& inventory ) {
  for( auto inv : enum_traits<e_unit_inventory>::values )
    if( inventory_to_commodity( inv ) ) inventory.erase( inv );
}

} // namespace

/****************************************************************
** Transformations
*****************************************************************/
void to_str( UnitTransformationResult const& o, string& out ) {
  out += fmt::format(
      "UnitTransformationResult{{\n"
      "  new_comp={},\n"
      "  modifier_deltas={},\n"
      "  commodity_deltas={}\n"
      "}}",
      o.new_comp, FmtVerticalMap{ o.modifier_deltas },
      FmtVerticalMap{ o.commodity_deltas } );
  // out += "UnitTransformationResult";
  // out += fmt::format(
  //     "{}",
  //     FmtVerticalMap{ unordered_map<string, string>{
  //         { "new_comp", fmt::to_string( o.new_comp ) },
  //         { "modifier_deltas", fmt::to_string( FmtVerticalMap{
  //                                  o.modifier_deltas } ) },
  //         { "commodity_deltas", fmt::to_string(
  //         FmtVerticalMap{
  //                                   o.commodity_deltas } ) },
  //     } } );
}

void to_str( UnitTransformationFromCommodityResult const& o,
             string&                                      out ) {
  out += fmt::format(
      "UnitTransformationFromCommodityResult{{new_comp={},"
      "modifier_deltas={},quantity_used={}}}",
      o.new_comp, FmtVerticalMap{ o.modifier_deltas },
      o.quantity_used );
  // out += "UnitTransformationFromCommodityResult";
  // out += fmt::format(
  //     "{}",
  //     FmtVerticalMap{ unordered_map<string, string>{
  //         { "new_comp", fmt::to_string( o.new_comp ) },
  //         { "modifier_deltas", fmt::to_string( FmtVerticalMap{
  //                                  o.modifier_deltas } ) },
  //         { "quantity_used",
  //           fmt::to_string( o.quantity_used ) } } } );
}

vector<UnitTransformationResult> possible_unit_transformations(
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
  vector<UnitTransformationResult> res;
  for( auto const& [new_type, mods] : mods_map ) {
    unordered_map<e_commodity, int> commodities = starting_comms;
    // First subtract commodities required by modifiers that re-
    // quire a fixed number of commodities.
    for( e_unit_type_modifier mod : mods ) {
      maybe<Commodity> comm =
          config_units.composition.modifier_traits[mod]
              .association
              .get_if<ModifierAssociation::commodity>()
              .member(
                  &ModifierAssociation::commodity::commodity );
      if( !comm.has_value() ) continue;
      DCHECK( comm->quantity > 0 );
      commodities[comm->type] -= comm->quantity;
    }
    // Next determine commodities required in inventory.
    unordered_map<e_unit_inventory, int> new_inventory =
        comp.inventory();
    // We remove these because we've already added them into the
    // starting_comms above.
    remove_commodities_from_inventory( new_inventory );
    for( e_unit_type_modifier mod : mods ) {
      auto inventory =
          config_units.composition.modifier_traits[mod]
              .association
              .get_if<ModifierAssociation::inventory>();
      if( !inventory.has_value() )
        // Does not require any inventory commodities.
        continue;
      maybe<e_commodity> comm =
          config_units.composition
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
      for( auto mod :
           enum_traits<e_unit_type_modifier>::values ) {
        if( old_mods.contains( mod ) &&
            !new_mods.contains( mod ) )
          modifier_deltas[mod] = e_unit_type_modifier_delta::del;
        else if( !old_mods.contains( mod ) &&
                 new_mods.contains( mod ) )
          modifier_deltas[mod] = e_unit_type_modifier_delta::add;
      }
      unordered_map<e_commodity, int> commodity_deltas;
      for( auto comm_type : enum_values<e_commodity>() ) {
        int orig_quantity =
            base::lookup( commodity_store, comm_type )
                .value_or( 0 );
        int new_quantity = commodities[comm_type];
        int delta        = new_quantity - orig_quantity;
        if( delta == 0 ) continue;
        commodity_deltas[comm_type] = delta;
      }

      res.push_back( UnitTransformationResult{
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

vector<UnitTransformationFromCommodityResult>
unit_receive_commodity( UnitComposition const& comp,
                        Commodity const&       commodity ) {
  vector<UnitTransformationResult> general_results =
      possible_unit_transformations(
          comp, { { commodity.type, commodity.quantity } } );
  vector<UnitTransformationFromCommodityResult> res;
  res.reserve( general_results.size() );
  for( UnitTransformationResult const& utr : general_results ) {
    if( utr.commodity_deltas.size() != 1 ) continue;
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
    // change is negative, because this function is only con-
    // cerned with *giving* a unit a commodity.
    if( quantity_delta > 0 ) continue;
    // quantity_delta is negative, meaning that it is being sub-
    // tracted from the commoodity store, which means that we
    // need to give it to the unit, which is what we want.
    res.push_back( UnitTransformationFromCommodityResult{
        .new_comp        = utr.new_comp,
        .modifier_deltas = utr.modifier_deltas,
        .quantity_used   = -quantity_delta } );
  }
  return res;
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
    vector<UnitTransformationResult>& input,
    bool                              independence_declared ) {
  if( independence_declared ) return;
  erase_if( input, L( requires_independence( _ ) ) );
}

void adjust_for_independence_status(
    vector<UnitTransformationFromCommodityResult>& input,
    bool independence_declared ) {
  if( independence_declared ) return;
  erase_if( input, L( requires_independence( _ ) ) );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = UnitComposition;

  auto u = st.usertype.create<UnitComposition>();

  u["base_type"]     = &U::base_type;
  u["type"]          = &U::type;
  u["type_obj"]      = &U::type_obj;
  u["with_new_type"] = &U::with_new_type;

  lua::table ucomp_tbl =
      lua::table::create_or_get( st["unit_composer"] );

  lua::table tbl_UnitComposition =
      lua::table::create_or_get( ucomp_tbl["UnitComposition"] );

  tbl_UnitComposition["create_with_type"] =
      [&]( e_unit_type type ) -> UnitComposition {
    return UnitComposition::create( type );
  };

  tbl_UnitComposition["create_with_type_obj"] =
      [&]( UnitType type ) -> UnitComposition {
    return UnitComposition::create( type );
  };
};

} // namespace

} // namespace rn
