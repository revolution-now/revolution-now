/****************************************************************
**promotion.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-10.
*
* Description: All things related to unit type promotion.
*
*****************************************************************/
#include "promotion.hpp"

// Revolution Now
#include "ustate.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit-composer.hpp"
#include "ss/unit.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool try_promote_unit_for_current_activity( SSConst const& ss,
                                            Unit& unit ) {
  if( !is_unit_human( unit.type_obj() ) ) return false;
  maybe<e_unit_activity> activity = current_activity_for_unit(
      ss.units, ss.colonies, unit.id() );
  if( !activity.has_value() ) return false;
  if( unit_attr( unit.base_type() ).expertise == *activity )
    return false;
  expect<UnitComposition> promoted =
      promoted_from_activity( unit.composition(), *activity );
  if( !promoted.has_value() ) return false;
  unit.change_type( *promoted );
  return true;
}

} // namespace rn
