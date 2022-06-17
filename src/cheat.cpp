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

} // namespace rn
