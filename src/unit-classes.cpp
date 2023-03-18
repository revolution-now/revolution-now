/****************************************************************
**unit-classes.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-18.
*
# Description: Types of units within a given unit class.
*
*****************************************************************/
#include "unit-classes.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
maybe<e_scout_type> scout_type( e_unit_type type ) {
  if( type == e_unit_type::scout )
    return e_scout_type::non_seasoned;
  if( type == e_unit_type::seasoned_scout )
    return e_scout_type::seasoned;
  return nothing;
}

maybe<e_pioneer_type> pioneer_type( e_unit_type type ) {
  if( type == e_unit_type::pioneer )
    return e_pioneer_type::non_hardy;
  if( type == e_unit_type::hardy_pioneer )
    return e_pioneer_type::hardy;
  return nothing;
}

maybe<e_missionary_type> missionary_type( UnitType type ) {
  if( type.type() != e_unit_type::missionary &&
      type.type() != e_unit_type::jesuit_missionary )
    return nothing;
  switch( type.base_type() ) {
    case e_unit_type::petty_criminal:
      return e_missionary_type::criminal;
    case e_unit_type::indentured_servant:
      return e_missionary_type::indentured;
    case e_unit_type::jesuit_colonist:
      return e_missionary_type::jesuit;
    default:
      return e_missionary_type::normal;
  }
}

} // namespace rn
