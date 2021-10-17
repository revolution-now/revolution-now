/****************************************************************
* Units Config File
*****************************************************************/
#ifndef UNITS_INL
#define UNITS_INL

#include "../../src/utype.hpp"

namespace rn {

using UnitTypeModifierTraitsMap = ExhaustiveEnumMap<
  e_unit_type_modifier,
  UnitTypeModifierTraits
>;

using UnitInventoryTraitsMap = ExhaustiveEnumMap<
  e_unit_inventory,
  UnitInventoryTraits
>;

CFG( units,
  FLD( UnitInventoryTraitsMap, inventory_traits )
  FLD( UnitTypeModifierTraitsMap, modifier_traits )
  FLD( UnitAttributesMap, unit_types )
)

}

#endif
