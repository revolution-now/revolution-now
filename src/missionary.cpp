/****************************************************************
**missionary.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-05.
*
* Description: All things related to missionaries.
*
*****************************************************************/
#include "missionary.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "unit.hpp"

// config
#include "config/unit-type.hpp"

// gs
#include "gs/colony.rds.hpp"
#include "gs/unit-type.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
bool can_bless_missionaries( Colony const& colony ) {
  return building_for_slot( colony,
                            e_colony_building_slot::crosses )
      .has_value();
}

bool unit_can_be_blessed( UnitType type ) {
  return is_unit_human( type );
}

void bless_as_missionary( Colony& colony, Unit& unit ) {
  strip_unit_to_base_type( unit, colony );
  UNWRAP_CHECK( ut, add_unit_type_modifiers(
                        unit.type_obj(),
                        { e_unit_type_modifier::blessing } ) );
  unit.change_type( UnitComposition::create( ut ) );
}

} // namespace rn
