/****************************************************************
**unit-composition.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-03.
*
* Description: TODO [FILL ME IN]
*
*****************************************************************/
#include "unit-composition.hpp"

// config
#include "config/unit-type.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

/****************************************************************
** UnitComposition
*****************************************************************/
valid_or<string> wrapped::UnitComposition::validate() const {
  // Validation: make sure that quantities of inventory items are
  // within range.
  for( e_unit_inventory type :
       refl::enum_values<e_unit_inventory> ) {
    int const quantity = inventory[type];
    if( quantity == 0 ) continue;
    UnitInventoryTraits const& traits =
        config_unit_type.composition.inventory_traits[type];
    REFL_VALIDATE( quantity >= traits.min_quantity,
                   "{} inventory must have at least {} items.",
                   type, traits.min_quantity );
    REFL_VALIDATE( quantity <= traits.max_quantity,
                   "{} inventory must have at most {} items.",
                   type, traits.max_quantity );
    REFL_VALIDATE(
        quantity % traits.multiple == 0,
        "{} inventory must come in multiples of {} items.", type,
        traits.multiple );
  }

  // Validation: make sure that the unit has all of the inventory
  // types that it is supposed to have.
  for( e_unit_inventory inv :
       config_unit_type.composition.unit_types[type.type()]
           .inventory_types )
    REFL_VALIDATE( inventory[inv] > 0,
                   "unit requires inventory type `{}' but it "
                   "was not provided.",
                   inv );

  // Validation: make sure that the unit has no more inventory
  // types than it is supposed to.
  for( auto [inv, q] : inventory )
    if( q > 0 )
      REFL_VALIDATE(
          config_unit_type.composition.unit_types[type.type()]
              .inventory_types.contains( inv ),
          "unit has inventory type `{}' but it is not in the "
          "list of allowed inventory types for that unit type.",
          inv );

  return base::valid;
}

UnitComposition::UnitComposition( UnitType type ) {
  UnitComposition::UnitInventoryMap inventory;
  for( e_unit_inventory inv :
       config_unit_type.composition.unit_types[type.type()]
           .inventory_types )
    inventory[inv] =
        config_unit_type.composition.inventory_traits[inv]
            .default_quantity;
  UNWRAP_CHECK( res, UnitComposition::create(
                         type, std::move( inventory ) ) );
  *this = res;
}

UnitComposition::UnitComposition( e_unit_type type )
  : UnitComposition( UnitType( type ) ) {}

expect<UnitComposition> UnitComposition::create(
    UnitType type, UnitInventoryMap inventory ) {
  auto inner = wrapped::UnitComposition{
    .type = type, .inventory = std::move( inventory ) };
  if( auto ok = inner.validate(); !ok ) return ok.error();
  return UnitComposition( std::move( inner ) );
}

expect<UnitComposition> UnitComposition::with_new_type(
    UnitType type ) const {
  return create( type, o_.inventory );
}

int UnitComposition::operator[]( e_unit_inventory inv ) const {
  return o_.inventory[inv];
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = UnitComposition;

  auto u = st.usertype.create<UnitComposition>();

  u["base_type"] = &U::base_type;
  u["type"]      = &U::type;
  u["type_obj"]  = []( U& o ) { return o.type_obj(); };
  // FIXME: Currently Lua does not know how to deal with the `ex-
  // pect` type that this returns. What we should probably do is
  // to create a wrapper whereby we could write:
  //
  //   u["with_new_type"] = expect_to_error( &U::with_new_type );
  //
  // Which would wrap the function in a new function that calls
  // it and then checks the result, and if it's an error then it
  // throws a Lua error, otherwise returns the contained type.
  // The alternative, of exposing base::expect in lua, might not
  // be trivial because it is not clear how to distringuish the
  // error state from the success state in a usable way.
  //
  // u["with_new_type"] = &U::with_new_type;

  lua::table ucomp_tbl =
      lua::table::create_or_get( st["unit_composition"] );

  lua::table tbl_UnitComposition =
      lua::table::create_or_get( ucomp_tbl["UnitComposition"] );

  tbl_UnitComposition["create_with_type"] =
      [&]( e_unit_type type ) -> UnitComposition {
    return type;
  };

  tbl_UnitComposition["create_with_type_obj"] =
      [&]( UnitType type ) -> UnitComposition { return type; };
};

} // namespace

} // namespace rn
