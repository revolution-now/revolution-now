/****************************************************************
**colony-buildings.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: All things related to colony buildings.
*
*****************************************************************/
#include "colony-buildings.hpp"

// Revolution Now
#include "colony.hpp"
#include "custom-house.hpp"
#include "production.hpp"

// config
#include "config/colony.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
e_colony_building_slot slot_for_building(
    e_colony_building building ) {
  switch( building ) {
    case e_colony_building::arsenal:
    case e_colony_building::magazine:
    case e_colony_building::armory:
      return e_colony_building_slot::muskets;

    case e_colony_building::iron_works:
    case e_colony_building::blacksmiths_shop:
    case e_colony_building::blacksmiths_house:
      return e_colony_building_slot::tools;

    case e_colony_building::lumber_mill:
    case e_colony_building::carpenters_shop:
      return e_colony_building_slot::hammers;

    case e_colony_building::fur_factory:
    case e_colony_building::fur_trading_post:
    case e_colony_building::fur_traders_house:
      return e_colony_building_slot::coats;

    case e_colony_building::rum_factory:
    case e_colony_building::rum_distillery:
    case e_colony_building::rum_distillers_house:
      return e_colony_building_slot::rum;

    case e_colony_building::cigar_factory:
    case e_colony_building::tobacconists_shop:
    case e_colony_building::tobacconists_house:
      return e_colony_building_slot::cigars;

    case e_colony_building::textile_mill:
    case e_colony_building::weavers_shop:
    case e_colony_building::weavers_house:
      return e_colony_building_slot::cloth;

    case e_colony_building::town_hall:
      return e_colony_building_slot::town_hall;

    case e_colony_building::newspaper:
    case e_colony_building::printing_press:
      return e_colony_building_slot::newspapers;

    case e_colony_building::university:
    case e_colony_building::college:
    case e_colony_building::schoolhouse:
      return e_colony_building_slot::schools;

    case e_colony_building::shipyard:
    case e_colony_building::drydock:
    case e_colony_building::docks:
      return e_colony_building_slot::offshore;

    case e_colony_building::stable:
      return e_colony_building_slot::horses;

    case e_colony_building::fortress:
    case e_colony_building::fort:
    case e_colony_building::stockade:
      return e_colony_building_slot::wall;

    case e_colony_building::warehouse_expansion:
    case e_colony_building::warehouse:
      return e_colony_building_slot::warehouses;

    case e_colony_building::cathedral:
    case e_colony_building::church:
      return e_colony_building_slot::crosses;

    case e_colony_building::custom_house:
      return e_colony_building_slot::custom_house;
  }
}

maybe<e_indoor_job> indoor_job_for_slot(
    e_colony_building_slot slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return e_indoor_job::muskets;
    case e_colony_building_slot::tools:
      return e_indoor_job::tools;
    case e_colony_building_slot::rum: //
      return e_indoor_job::rum;
    case e_colony_building_slot::cloth:
      return e_indoor_job::cloth;
    case e_colony_building_slot::coats: //
      return e_indoor_job::coats;
    case e_colony_building_slot::cigars:
      return e_indoor_job::cigars;
    case e_colony_building_slot::hammers:
      return e_indoor_job::hammers;
    case e_colony_building_slot::town_hall:
      return e_indoor_job::bells;
    case e_colony_building_slot::newspapers: //
      return nothing;
    case e_colony_building_slot::schools:
      return e_indoor_job::teacher;
    case e_colony_building_slot::offshore:   //
      return nothing;
    case e_colony_building_slot::horses:     //
      return nothing;
    case e_colony_building_slot::wall:       //
      return nothing;
    case e_colony_building_slot::warehouses: //
      return nothing;
    case e_colony_building_slot::crosses:
      return e_indoor_job::crosses;
    case e_colony_building_slot::custom_house: //
      return nothing;
  }
}

e_colony_building_slot slot_for_indoor_job( e_indoor_job job ) {
  switch( job ) {
    case e_indoor_job::bells:
      return e_colony_building_slot::town_hall;
    case e_indoor_job::crosses:
      return e_colony_building_slot::crosses;
    case e_indoor_job::hammers:
      return e_colony_building_slot::hammers;
    case e_indoor_job::rum: //
      return e_colony_building_slot::rum;
    case e_indoor_job::cigars:
      return e_colony_building_slot::cigars;
    case e_indoor_job::cloth:
      return e_colony_building_slot::cloth;
    case e_indoor_job::coats:
      return e_colony_building_slot::coats;
    case e_indoor_job::tools:
      return e_colony_building_slot::tools;
    case e_indoor_job::muskets:
      return e_colony_building_slot::muskets;
    case e_indoor_job::teacher:
      return e_colony_building_slot::schools;
  }
}

maybe<e_colony_building> building_for_slot(
    Colony const& colony, e_colony_building_slot slot ) {
  refl::enum_map<e_colony_building, bool> const& buildings =
      colony.buildings;
  for( e_colony_building building : buildings_for_slot( slot ) )
    if( buildings[building] ) //
      return building;
  return nothing;
}

vector<e_colony_building> const& buildings_for_slot(
    e_colony_building_slot slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets: {
      static vector<e_colony_building> const bs{
          e_colony_building::arsenal,
          e_colony_building::magazine,
          e_colony_building::armory,
      };
      return bs;
    }
    case e_colony_building_slot::tools: {
      static vector<e_colony_building> const bs{
          e_colony_building::iron_works,
          e_colony_building::blacksmiths_shop,
          e_colony_building::blacksmiths_house,
      };
      return bs;
    }
    case e_colony_building_slot::rum: {
      static vector<e_colony_building> const bs{
          e_colony_building::rum_factory,
          e_colony_building::rum_distillery,
          e_colony_building::rum_distillers_house,
      };
      return bs;
    }
    case e_colony_building_slot::cloth: {
      static vector<e_colony_building> const bs{
          e_colony_building::textile_mill,
          e_colony_building::weavers_shop,
          e_colony_building::weavers_house,
      };
      return bs;
    }
    case e_colony_building_slot::coats: {
      static vector<e_colony_building> const bs{
          e_colony_building::fur_factory,
          e_colony_building::fur_trading_post,
          e_colony_building::fur_traders_house,
      };
      return bs;
    }
    case e_colony_building_slot::cigars: {
      static vector<e_colony_building> const bs{
          e_colony_building::cigar_factory,
          e_colony_building::tobacconists_shop,
          e_colony_building::tobacconists_house,
      };
      return bs;
    }
    case e_colony_building_slot::hammers: {
      static vector<e_colony_building> const bs{
          e_colony_building::lumber_mill,
          e_colony_building::carpenters_shop,
      };
      return bs;
    }
    case e_colony_building_slot::town_hall: {
      static vector<e_colony_building> const bs{
          e_colony_building::town_hall,
      };
      return bs;
    }
    case e_colony_building_slot::newspapers: {
      static vector<e_colony_building> const bs{
          e_colony_building::newspaper,
          e_colony_building::printing_press,
      };
      return bs;
    }
    case e_colony_building_slot::schools: {
      static vector<e_colony_building> const bs{
          e_colony_building::university,
          e_colony_building::college,
          e_colony_building::schoolhouse,
      };
      return bs;
    }
    case e_colony_building_slot::offshore: {
      static vector<e_colony_building> const bs{
          e_colony_building::shipyard,
          e_colony_building::drydock,
          e_colony_building::docks,
      };
      return bs;
    }
    case e_colony_building_slot::horses: {
      static vector<e_colony_building> const bs{
          e_colony_building::stable,
      };
      return bs;
    }
    case e_colony_building_slot::wall: {
      static vector<e_colony_building> const bs{
          e_colony_building::fortress,
          e_colony_building::fort,
          e_colony_building::stockade,
      };
      return bs;
    }
    case e_colony_building_slot::warehouses: {
      static vector<e_colony_building> const bs{
          e_colony_building::warehouse_expansion,
          e_colony_building::warehouse,
      };
      return bs;
    }
    case e_colony_building_slot::crosses: {
      static vector<e_colony_building> const bs{
          e_colony_building::cathedral,
          e_colony_building::church,
      };
      return bs;
    }
    case e_colony_building_slot::custom_house: {
      static vector<e_colony_building> const bs{
          e_colony_building::custom_house,
      };
      return bs;
    }
  }
}

bool colony_has_building_level( Colony const&     colony,
                                e_colony_building building ) {
  e_colony_building_slot const slot =
      slot_for_building( building );
  vector<e_colony_building> const& possible_buildings =
      buildings_for_slot( slot );
  for( e_colony_building possible_building :
       possible_buildings ) {
    if( colony.buildings[possible_building] ) return true;
    if( possible_building == building ) return false;
  }
  FATAL(
      "colony building `{}` not in list of buildings for slot "
      "`{}`.",
      building, slot );
}

int colony_warehouse_capacity( Colony const& colony ) {
  maybe<e_colony_building> building = building_for_slot(
      colony, e_colony_building_slot::warehouses );
  if( !building.has_value() )
    return config_colony.warehouses.default_max_quantity;
  if( building == e_colony_building::warehouse )
    return config_colony.warehouses.warehouse_max_quantity;
  if( building == e_colony_building::warehouse_expansion )
    return config_colony.warehouses
        .warehouse_expansion_max_quantity;
  SHOULD_NOT_BE_HERE;
}

e_unit_activity activity_for_indoor_job( e_indoor_job job ) {
  switch( job ) {
    case e_indoor_job::bells:
      return e_unit_activity::bell_ringing;
    case e_indoor_job::crosses:
      return e_unit_activity::preaching;
    case e_indoor_job::hammers:
      return e_unit_activity::carpentry;
    case e_indoor_job::rum:
      return e_unit_activity::rum_distilling;
    case e_indoor_job::cigars:
      return e_unit_activity::tobacconistry;
    case e_indoor_job::cloth:
      return e_unit_activity::weaving;
    case e_indoor_job::coats:
      return e_unit_activity::fur_trading;
    case e_indoor_job::tools:
      return e_unit_activity::blacksmithing;
    case e_indoor_job::muskets:
      return e_unit_activity::gunsmithing;
    case e_indoor_job::teacher:
      return e_unit_activity::teaching;
  }
}

maybe<e_school_type> school_type_from_building(
    e_colony_building building ) {
  switch( building ) {
    case e_colony_building::schoolhouse:
      return e_school_type::schoolhouse;
    case e_colony_building::college:
      return e_school_type::college;
    case e_colony_building::university:
      return e_school_type::university;
    default:
      break;
  }
  return nothing;
}

e_colony_building building_for_school_type(
    e_school_type school_type ) {
  switch( school_type ) {
    case e_school_type::schoolhouse:
      return e_colony_building::schoolhouse;
    case e_school_type::college:
      return e_colony_building::college;
    case e_school_type::university:
      return e_colony_building::university;
  }
}

int max_workers_for_building( e_colony_building building ) {
  if( maybe<e_school_type> school_type =
          school_type_from_building( building );
      school_type.has_value() )
    return config_colony.teaching.max_teachers[*school_type];
  e_colony_building_slot const slot =
      slot_for_building( building );
  maybe<e_indoor_job> const indoor_job =
      indoor_job_for_slot( slot );
  if( !indoor_job.has_value() ) return 0;
  return config_colony.max_workers_per_building;
}

void add_colony_building( Colony&           colony,
                          e_colony_building building ) {
  if( colony.buildings[building] ) return;
  colony.buildings[building] = true;
  switch( building ) {
    case e_colony_building::custom_house:
      set_default_custom_house_state( colony );
      break;
    default:
      break;
  }
}

e_colony_building barricade_type_to_colony_building(
    e_colony_barricade_type barricade ) {
  switch( barricade ) {
    case e_colony_barricade_type::stockade:
      return e_colony_building::stockade;
    case e_colony_barricade_type::fort:
      return e_colony_building::fort;
    case e_colony_barricade_type::fortress:
      return e_colony_building::fortress;
  }
}

maybe<e_colony_barricade_type> barricade_for_colony(
    Colony const& colony ) {
  maybe<e_colony_building> const building =
      building_for_slot( colony, e_colony_building_slot::wall );
  if( !building.has_value() ) return nothing;
  switch( *building ) {
    case e_colony_building::stockade:
      return e_colony_barricade_type::stockade;
    case e_colony_building::fort:
      return e_colony_barricade_type::fort;
    case e_colony_building::fortress:
      return e_colony_barricade_type::fortress;
    default:
      FATAL( "unexpected building type for barricade: {}.",
             *building );
  }
}

} // namespace rn
