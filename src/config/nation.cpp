/****************************************************************
**nation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-31.
*
# Description: Nation-specific config info.
*
*****************************************************************/
#include "nation.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

namespace {

bool valid_pioneer( e_unit_type u ) {
  switch( u ) {
    case e_unit_type::pioneer:
    case e_unit_type::hardy_pioneer: return true;
    default: return false;
  }
}

bool valid_ship( e_unit_type u ) {
  switch( u ) {
    case e_unit_type::caravel:
    case e_unit_type::merchantman:
    case e_unit_type::galleon:
    case e_unit_type::privateer:
    case e_unit_type::frigate:
    case e_unit_type::man_o_war: return true;
    default: return false;
  }
}

} // namespace

base::valid_or<string> SpecialAbility::validate() const {
  // NOTE: unfortunately we cannot validate the properties of the
  // unit type fields because that would require config data from
  // another config module which is necessarily available while
  // this config is being loaded.

  REFL_VALIDATE( crosses_needed > 0.0,
                 "crosses multiplier must be > 0." );

  REFL_VALIDATE( valid_ship( starting_ship ),
                 "starting ship unit must be a valid ship." );

  REFL_VALIDATE( valid_pioneer( starting_pioneer ),
                 "starting pioneer unit must be either a "
                 "`pioneer` or `hardy_pioneer`." );

  REFL_VALIDATE(
      native_alarm_multiplier >= 0.0,
      "native_alarm_multiplier multiplier must be >= 0." );

  REFL_VALIDATE(
      price_movement_multiplier >= 0.0,
      "price_movement_multiplier multiplier must be >= 0." );

  REFL_VALIDATE(
      native_settlement_attack_bonus >= 0,
      "native_settlement_attack_bonus must be >= 0." );

  return base::valid;
}

} // namespace rn
