/****************************************************************
**native-enums.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-19.
*
* Description: Enums associated with the natives.
*
*****************************************************************/
#include "native-enums.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
e_unit_activity activity_for_native_skill(
    e_native_skill skill ) {
  switch( skill ) {
    case e_native_skill::farming:
      return e_unit_activity::farming;
    case e_native_skill::fishing:
      return e_unit_activity::fishing;
    case e_native_skill::sugar_planting:
      return e_unit_activity::sugar_planting;
    case e_native_skill::tobacco_planting:
      return e_unit_activity::tobacco_planting;
    case e_native_skill::cotton_planting:
      return e_unit_activity::cotton_planting;
    case e_native_skill::fur_trapping:
      return e_unit_activity::fur_trapping;
    case e_native_skill::ore_mining:
      return e_unit_activity::ore_mining;
    case e_native_skill::silver_mining:
      return e_unit_activity::silver_mining;
    case e_native_skill::fur_trading:
      return e_unit_activity::fur_trading;
    case e_native_skill::scouting:
      return e_unit_activity::scouting;
  }
}

base::maybe<e_native_skill> native_skill_for_activity(
    e_unit_activity activity ) {
  switch( activity ) {
    case e_unit_activity::farming: //
      return e_native_skill::farming;
    case e_unit_activity::fishing: //
      return e_native_skill::fishing;
    case e_unit_activity::sugar_planting: //
      return e_native_skill::sugar_planting;
    case e_unit_activity::tobacco_planting: //
      return e_native_skill::tobacco_planting;
    case e_unit_activity::cotton_planting: //
      return e_native_skill::cotton_planting;
    case e_unit_activity::fur_trapping: //
      return e_native_skill::fur_trapping;
    case e_unit_activity::lumberjacking: //
      return base::nothing;
    case e_unit_activity::ore_mining: //
      return e_native_skill::ore_mining;
    case e_unit_activity::silver_mining: //
      return e_native_skill::silver_mining;
    case e_unit_activity::carpentry: //
      return base::nothing;
    case e_unit_activity::rum_distilling: //
      return base::nothing;
    case e_unit_activity::tobacconistry: //
      return base::nothing;
    case e_unit_activity::weaving: //
      return base::nothing;
    case e_unit_activity::fur_trading: //
      return e_native_skill::fur_trading;
    case e_unit_activity::blacksmithing: //
      return base::nothing;
    case e_unit_activity::gunsmithing: //
      return base::nothing;
    case e_unit_activity::fighting: //
      return base::nothing;
    case e_unit_activity::pioneering: //
      return base::nothing;
    case e_unit_activity::scouting: //
      return e_native_skill::scouting;
    case e_unit_activity::missioning: //
      return base::nothing;
    case e_unit_activity::bell_ringing: //
      return base::nothing;
    case e_unit_activity::preaching: //
      return base::nothing;
    case e_unit_activity::teaching: //
      return base::nothing;
  }
}

} // namespace rn
