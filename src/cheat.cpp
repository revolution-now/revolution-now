/****************************************************************
**cheat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-16.
*
* Description: Implements cheat mode.
*
*****************************************************************/
#include "cheat.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony.hpp"
#include "igui.hpp"
#include "unit.hpp"
#include "ustate.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/rn.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

#define CO_RETURN_IF_NO_CHEAT \
  if( !config_rn.cheat_functions_enabled ) co_return

#define RETURN_IF_NO_CHEAT \
  if( !config_rn.cheat_functions_enabled ) return

namespace {

bool can_remove_building( Colony const&     colony,
                          e_colony_building building ) {
  if( !colony.buildings()[building] ) return true;
  e_colony_building_slot slot = slot_for_building( building );
  UNWRAP_CHECK( foremost, building_for_slot( colony, slot ) );
  if( foremost != building ) return true;
  maybe<e_indoor_job> job = indoor_job_for_slot( slot );
  if( !job.has_value() ) return true;
  return colony.indoor_jobs()[*job].empty();
}

} // namespace

/****************************************************************
** In Colony View
*****************************************************************/
wait<> cheat_colony_buildings( Colony& colony, IGui& gui ) {
  CO_RETURN_IF_NO_CHEAT;
  maybe<e_cheat_colony_buildings_option> mode =
      co_await gui
          .enum_choice<e_cheat_colony_buildings_option>();
  if( !mode.has_value() ) co_return;
  switch( *mode ) {
    case e_cheat_colony_buildings_option::give_all_buildings:
      for( e_colony_building building :
           refl::enum_values<e_colony_building> )
        colony.add_building( building );
      break;
    case e_cheat_colony_buildings_option::remove_all_buildings: {
      bool can_not_remove_all = false;
      for( e_colony_building building :
           refl::enum_values<e_colony_building> ) {
        if( can_remove_building( colony, building ) )
          colony.rm_building( building );
        else
          can_not_remove_all = true;
      }
      if( can_not_remove_all )
        co_await gui.message_box(
            "Unable to remove all buildings since some of them "
            "contain colonists." );
      break;
    }
    case e_cheat_colony_buildings_option::set_default_buildings:
      for( e_colony_building building :
           refl::enum_values<e_colony_building> )
        colony.rm_building( building );
      for( e_colony_building building :
           config_colony.initial_colony_buildings )
        colony.add_building( building );
      break;
    case e_cheat_colony_buildings_option::add_one_building: {
      maybe<e_colony_building> building =
          co_await gui.enum_choice<e_colony_building>(
              /*sort=*/true );
      if( building.has_value() )
        colony.add_building( *building );
      break;
    }
    case e_cheat_colony_buildings_option::remove_one_building: {
      maybe<e_colony_building> building =
          co_await gui.enum_choice<e_colony_building>(
              /*sort=*/true );
      if( !building.has_value() ) co_return;
      if( !can_remove_building( colony, *building ) ) {
        co_await gui.message_box(
            "Cannot remove this building while it contains "
            "colonists working in it." );
        co_return;
      }
      colony.rm_building( *building );
      break;
    }
  }
}

void cheat_upgrade_unit_expertise(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, Unit& unit ) {
  RETURN_IF_NO_CHEAT;
  if( !is_unit_human( unit.type_obj() ) ) return;

  maybe<e_unit_activity> activity = current_activity_for_unit(
      units_state, colonies_state, unit.id() );

  if( activity.has_value() ) {
    maybe<UnitType> promoted =
        promoted_unit_type( unit.type_obj(), *activity );
    if( !promoted.has_value() ) {
      // This is for when we have a unit that is already an ex-
      // pert at something other than the given activity, we want
      // to switch them to being an expert at the given activity.
      promoted = promoted_unit_type(
          UnitType::create( e_unit_type::free_colonist ),
          *activity );
      if( !promoted.has_value() ) return;
    }
    unit.change_type( UnitComposition::create( *promoted ) );
    return;
  }

  // No activity. without an activity, the only people we know
  // how to upgrade are petty criminals and indentured servants.
  switch( unit.type() ) {
    case e_unit_type::petty_criminal:
      unit.change_type( UnitComposition::create(
          e_unit_type::indentured_servant ) );
      return;
    case e_unit_type::indentured_servant:
      unit.change_type( UnitComposition::create(
          e_unit_type::free_colonist ) );
      break;
    default: return;
  }
}

void cheat_downgrade_unit_expertise( Unit& unit ) {
  RETURN_IF_NO_CHEAT;
  if( !is_unit_human( unit.type_obj() ) ) return;

  if( !unit.desc().is_derived ) {
    e_unit_type new_type = {};
    switch( unit.type() ) {
      case e_unit_type::petty_criminal: return;
      case e_unit_type::indentured_servant:
        new_type = e_unit_type::petty_criminal;
        break;
      case e_unit_type::free_colonist:
        new_type = e_unit_type::indentured_servant;
        break;
      default: new_type = e_unit_type::free_colonist; break;
    }
    unit.change_type( UnitComposition::create( new_type ) );
    return;
  }

  // Derived type.
  switch( unit.base_type() ) {
    case e_unit_type::petty_criminal: return;
    case e_unit_type::indentured_servant: {
      UNWRAP_CHECK(
          ut, UnitType::create( unit.type(),
                                e_unit_type::petty_criminal ) );
      UNWRAP_CHECK( comp,
                    UnitComposition::create(
                        ut, unit.composition().inventory() ) );
      unit.change_type( comp );
      return;
    }
    case e_unit_type::free_colonist: {
      UNWRAP_CHECK( ut, UnitType::create(
                            unit.type(),
                            e_unit_type::indentured_servant ) );
      UNWRAP_CHECK( comp,
                    UnitComposition::create(
                        ut, unit.composition().inventory() ) );
      unit.change_type( comp );
      return;
    }
    default:
      maybe<UnitType> cleared =
          cleared_expertise( unit.type_obj() );
      if( !cleared.has_value() )
        // not sure what to do here.
        return;
      UNWRAP_CHECK(
          comp, UnitComposition::create(
                    *cleared, unit.composition().inventory() ) );
      unit.change_type( comp );
      return;
  }
}

} // namespace rn
