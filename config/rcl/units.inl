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

CFG( units,
  FLD( UnitTypeModifierTraitsMap, modifier_traits )
  FLD( UnitAttributesMap, unit_types )
)

}

#endif
