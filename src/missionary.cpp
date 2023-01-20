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
#include "config/missionary.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

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

void bless_as_missionary( Player const& player, Colony& colony,
                          Unit& unit ) {
  strip_unit_to_base_type( player, unit, colony );
  UNWRAP_CHECK( ut, add_unit_type_modifiers(
                        unit.type_obj(),
                        { e_unit_type_modifier::blessing } ) );
  unit.change_type( player, UnitComposition::create( ut ) );
}

bool is_missionary( e_unit_type type ) {
  return missionary_type( UnitType::create( type ) ).has_value();
}

maybe<double> probability_dwelling_produces_convert_on_attack(
    SSConst const& ss, Player const& player_attacking,
    DwellingId dwelling_id ) {
  maybe<UnitId> const missionary_id =
      ss.units.missionary_from_dwelling( dwelling_id );
  if( !missionary_id.has_value() ) return nothing;
  Unit const& missionary_unit =
      ss.units.unit_for( *missionary_id );
  if( missionary_unit.nation() != player_attacking.nation )
    return nothing;
  UNWRAP_CHECK( missionary_type,
                missionary_type( missionary_unit.type_obj() ) );
  return config_missionary.type[missionary_type]
      .convert_on_attack.probability;
}

} // namespace rn
