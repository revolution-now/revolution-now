/****************************************************************
**construction.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-20.
*
* Description: All things related to colony construction.
*
*****************************************************************/
#include "construction.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "igui.hpp"

// config
#include "config/colony.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-type.hpp"

// refl
#include "refl/to-str.hpp"

// Abseil
#include "absl/strings/str_replace.h"

using namespace std;

using ::rn::config::colony::construction_requirements;

namespace rn {

namespace {

string fmt_construction( string const& name,
                         string const& needed ) {
  string parens = fmt::format( "({})", needed );
  return fmt::format( "{:<22} {}", name, needed );
}

string fmt_construction(
    string const&                    name,
    construction_requirements const& materials ) {
  string needed =
      fmt::format( "{:>3} hammers", materials.hammers );
  if( materials.tools != 0 )
    needed += fmt::format( ", {:>3} tools", materials.tools );
  return fmt_construction( name, needed );
}

void adjust_materials( Colony const&              colony,
                       construction_requirements& materials ) {
  materials.hammers =
      std::max( materials.hammers - colony.hammers, 0 );
  materials.tools = std::max(
      materials.tools - colony.commodities[e_commodity::tools],
      0 );
}

string fmt_building( Colony const&     colony,
                     e_colony_building building ) {
  construction_requirements requirements =
      config_colony.requirements_for_building[building];
  adjust_materials( colony, requirements );
  return fmt_construction(
      construction_name(
          Construction::building{ .what = building } ),
      requirements );
}

string fmt_unit( Colony const& colony, e_unit_type type ) {
  maybe<construction_requirements> requirements =
      config_colony.requirements_for_unit[type];
  CHECK( requirements.has_value() );
  adjust_materials( colony, *requirements );
  return fmt_construction(
      construction_name( Construction::unit{ .type = type } ),
      *requirements );
}

// Note that the bordering water tile does not need to have ocean
// access (that's the behavior from the original game).
bool colony_borders_water( SSConst const& ss,
                           Colony const&  colony ) {
  for( e_direction d : refl::enum_values<e_direction> )
    if( ss.terrain.total_square_at( colony.location.moved( d ) )
            .surface == e_surface::water )
      return true;
  return false;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
string construction_name( Construction_t const& construction ) {
  switch( construction.to_enum() ) {
    using namespace Construction;
    case e::building: {
      auto& o = construction.get<building>();
      return config_colony.building_display_names[o.what];
    }
    case e::unit: {
      auto& o = construction.get<unit>();
      return unit_attr( o.type ).name;
    }
  }
}

wait<> select_colony_construction( SSConst const& ss,
                                   Colony& colony, IGui& gui ) {
  static string const kNoProductionKey = "none";
  ChoiceConfig config{ .msg = "Select One", .options = {} };
  config.options.push_back(
      ChoiceConfigOption{ .key          = kNoProductionKey,
                          .display_name = "(no production)" } );
  maybe<int> initial_selection;
  int const  population = colony_population( colony );
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );
  bool const has_water = colony_borders_water( ss, colony );
  for( e_colony_building building :
       refl::enum_values<e_colony_building> ) {
    if( colony.buildings[building] ) continue;
    auto const& requirements =
        config_colony.requirements_for_building[building];
    if( population < requirements.minimum_population ) continue;
    if( requirements.required_building.has_value() &&
        !colony_has_building_level(
            colony, *requirements.required_building ) )
      continue;
    if( requirements.required_father.has_value() &&
        !player.fathers.has[*requirements.required_father] )
      continue;
    if( requirements.requires_water && !has_water ) continue;
    config.options.push_back( ChoiceConfigOption{
        .key          = fmt::to_string( building ),
        .display_name = fmt_building( colony, building ) } );
    if( colony.construction.has_value() &&
        colony.construction ==
            Construction_t{
                Construction::building{ .what = building } } )
      initial_selection = config.options.size() - 1;
  }
  for( e_unit_type type : refl::enum_values<e_unit_type> ) {
    if( !config_colony.requirements_for_unit[type].has_value() )
      continue;
    auto const& requirements =
        *config_colony.requirements_for_unit[type];
    if( population < requirements.minimum_population ) continue;
    if( requirements.required_building.has_value() &&
        !colony_has_building_level(
            colony, *requirements.required_building ) )
      continue;
    if( requirements.required_father.has_value() &&
        !player.fathers.has[*requirements.required_father] )
      continue;
    if( requirements.requires_water && !has_water ) continue;
    config.options.push_back( ChoiceConfigOption{
        .key          = fmt::to_string( type ),
        .display_name = fmt_unit( colony, type ) } );
    if( colony.construction.has_value() &&
        colony.construction ==
            Construction_t{
                Construction::unit{ .type = type } } )
      initial_selection = config.options.size() - 1;
  }
  CHECK( !initial_selection.has_value() ||
         *initial_selection >= 0 );
  config.initial_selection = initial_selection;
  maybe<string> what       = co_await gui.choice( config );
  if( !what.has_value() ) co_return;

  if( what == kNoProductionKey ) {
    colony.construction.reset();
    co_return;
  }

  for( e_colony_building building :
       refl::enum_values<e_colony_building> ) {
    if( what == fmt::to_string( building ) ) {
      colony.construction =
          Construction::building{ .what = building };
      co_return;
    }
  }

  for( e_unit_type type : refl::enum_values<e_unit_type> ) {
    if( what == fmt::to_string( type ) ) {
      colony.construction = Construction::unit{ .type = type };
      co_return;
    }
  }

  FATAL( "unknown building name {}.", *what );
}

} // namespace rn
