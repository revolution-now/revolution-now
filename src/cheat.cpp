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
#include "colony-evolve.hpp"
#include "igui.hpp"
#include "logger.hpp"
#include "ts.hpp"
#include "unit.hpp"
#include "ustate.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/rn.rds.hpp"
#include "config/unit-type.hpp"

// gs
#include "ss/colony.hpp"
#include "ss/players.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

#define CO_RETURN_IF_NO_CHEAT \
  if( !config_rn.cheat_functions_enabled ) co_return

#define RETURN_IF_NO_CHEAT \
  if( !config_rn.cheat_functions_enabled ) return

namespace {

bool can_remove_building( Colony const&     colony,
                          e_colony_building building ) {
  if( !colony.buildings[building] ) return true;
  e_colony_building_slot slot = slot_for_building( building );
  UNWRAP_CHECK( foremost, building_for_slot( colony, slot ) );
  if( foremost != building ) return true;
  maybe<e_indoor_job> job = indoor_job_for_slot( slot );
  if( !job.has_value() ) return true;
  return colony.indoor_jobs[*job].empty();
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
        colony.buildings[building] = true;
      break;
    case e_cheat_colony_buildings_option::remove_all_buildings: {
      bool can_not_remove_all = false;
      for( e_colony_building building :
           refl::enum_values<e_colony_building> ) {
        if( can_remove_building( colony, building ) )
          colony.buildings[building] = false;
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
      colony.buildings = config_colony.initial_colony_buildings;
      break;
    case e_cheat_colony_buildings_option::add_one_building: {
      maybe<e_colony_building> building =
          co_await gui.enum_choice<e_colony_building>(
              /*sort=*/true );
      if( building.has_value() )
        colony.buildings[*building] = true;
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
      colony.buildings[*building] = false;
      break;
    }
  }
}

void cheat_upgrade_unit_expertise(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, Unit& unit ) {
  RETURN_IF_NO_CHEAT;
  UnitType const original_type = unit.type_obj();
  SCOPE_EXIT(
      lg.debug( "{} --> {}", original_type, unit.type_obj() ) );
  if( !is_unit_human( unit.type_obj() ) ) return;

  // First just use the normal game logic to attempt a promotion.
  // This will work in many cases but not all. It won't do a pro-
  // motion when the unit has no activity and it won't change an
  // expert's type to be a new expert, both things that we will
  // want to do with this cheat feature.
  if( try_promote_unit_for_current_activity(
          units_state, colonies_state, unit ) )
    return;

  maybe<e_unit_activity> activity = current_activity_for_unit(
      units_state, colonies_state, unit.id() );

  if( activity.has_value() ) {
    if( unit_attr( unit.base_type() ).expertise == *activity )
      return;
    // This is for when we have a unit that is already an expert
    // at something other than the given activity, we want to
    // switch them to being an expert at the given activity.
    UnitType to_promote;
    if( unit.desc().is_derived )
      // Take the canonical version of the thing try to promote
      // it (this is needed if it's a derived type).
      to_promote = UnitType::create( unit.type() );
    else
      // Not derived, so just try promoting a free colonist.
      to_promote =
          UnitType::create( e_unit_type::free_colonist );
    maybe<UnitType> new_ut =
        promoted_unit_type( to_promote, *activity );
    if( !new_ut.has_value() ) return;
    expect<UnitComposition> promoted =
        UnitComposition::create( *new_ut );
    if( !promoted.has_value() ) return;
    CHECK( promoted.has_value() );
    unit.change_type( *promoted );
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
  UnitType const original_type = unit.type_obj();
  SCOPE_EXIT(
      lg.debug( "{} --> {}", original_type, unit.type_obj() ) );
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

void cheat_create_new_colonist( UnitsState&   units_state,
                                IMapUpdater&  map_updater,
                                Colony const& colony ) {
  RETURN_IF_NO_CHEAT;
  create_unit_on_map_non_interactive(
      units_state, map_updater, colony.nation,
      UnitComposition::create( e_unit_type::free_colonist ),
      colony.location );
}

void cheat_increase_commodity( Colony&     colony,
                               e_commodity type ) {
  RETURN_IF_NO_CHEAT;
  int& quantity = colony.commodities[type];
  quantity += 50;
  quantity = quantity - ( quantity % 50 );
}

void cheat_decrease_commodity( Colony&     colony,
                               e_commodity type ) {
  RETURN_IF_NO_CHEAT;
  int& quantity = colony.commodities[type];
  if( quantity % 50 != 0 )
    quantity -= ( quantity % 50 );
  else
    quantity -= 50;
  quantity = std::max( quantity, 0 );
}

void cheat_advance_colony_one_turn( SS& ss, TS& ts,
                                    Colony& colony ) {
  RETURN_IF_NO_CHEAT;
  lg.debug( "advancing colony {}. notifications:", colony.name );
  ColonyEvolution ev = evolve_colony_one_turn( ss, ts, colony );
  for( ColonyNotification_t const& notification :
       ev.notifications )
    lg.debug( "{}", notification );
  // NOTE: we will not starve the colony here since this is just
  // a cheat/debug feature. We'll just log that it was supposed
  // to happen.
  if( ev.colony_disappeared ) lg.debug( "colony has starved." );
}

wait<> cheat_create_unit_on_map( SS& ss, TS& ts, e_nation nation,
                                 Coord tile ) {
  CO_RETURN_IF_NO_CHEAT;
  static refl::enum_map<e_cheat_unit_creation_categories,
                        vector<e_unit_type>> const categories{
      { e_cheat_unit_creation_categories::basic_colonists,
        {
            e_unit_type::free_colonist,
            e_unit_type::indentured_servant,
            e_unit_type::petty_criminal,
            e_unit_type::native_convert,
        } },
      { e_cheat_unit_creation_categories::modified_colonists,
        {
            e_unit_type::pioneer,
            e_unit_type::hardy_pioneer,
            e_unit_type::missionary,
            e_unit_type::jesuit_missionary,
            e_unit_type::scout,
            e_unit_type::seasoned_scout,
        } },
      { e_cheat_unit_creation_categories::expert_colonists,
        {
            e_unit_type::expert_farmer,
            e_unit_type::expert_fisherman,
            e_unit_type::expert_sugar_planter,
            e_unit_type::expert_tobacco_planter,
            e_unit_type::expert_cotton_planter,
            e_unit_type::expert_fur_trapper,
            e_unit_type::expert_lumberjack,
            e_unit_type::expert_ore_miner,
            e_unit_type::expert_silver_miner,
            e_unit_type::master_carpenter,
            e_unit_type::master_rum_distiller,
            e_unit_type::master_tobacconist,
            e_unit_type::master_weaver,
            e_unit_type::master_fur_trader,
            e_unit_type::master_blacksmith,
            e_unit_type::master_gunsmith,
            e_unit_type::elder_statesman,
            e_unit_type::firebrand_preacher,
            e_unit_type::hardy_colonist,
            e_unit_type::jesuit_colonist,
            e_unit_type::seasoned_colonist,
            e_unit_type::veteran_colonist,
        } },
      { e_cheat_unit_creation_categories::armies,
        {
            e_unit_type::soldier,
            e_unit_type::dragoon,
            e_unit_type::veteran_soldier,
            e_unit_type::veteran_dragoon,
            e_unit_type::continental_army,
            e_unit_type::continental_cavalry,
            e_unit_type::regular,
            e_unit_type::cavalry,
            e_unit_type::artillery,
            e_unit_type::damaged_artillery,
        } },
      { e_cheat_unit_creation_categories::ships,
        {
            e_unit_type::caravel,
            e_unit_type::merchantman,
            e_unit_type::galleon,
            e_unit_type::privateer,
            e_unit_type::frigate,
            e_unit_type::man_o_war,
        } },
      { e_cheat_unit_creation_categories::miscellaneous,
        {
            e_unit_type::wagon_train,
            e_unit_type::small_treasure,
            e_unit_type::large_treasure,
        } },
  };
  maybe<e_cheat_unit_creation_categories> category =
      co_await ts.gui
          .enum_choice<e_cheat_unit_creation_categories>();
  if( !category.has_value() ) co_return;
  maybe<e_unit_type> type =
      co_await ts.gui.partial_enum_choice<e_unit_type>(
          categories[*category] );
  if( !type.has_value() ) co_return;
  UNWRAP_CHECK( player, ss.players.players[nation] );
  maybe<UnitId> unit_id = co_await create_unit_on_map(
      ss.units, ss.terrain, player, ss.settings, ts.gui,
      ts.map_updater, UnitComposition::create( *type ), tile );
}

} // namespace rn
