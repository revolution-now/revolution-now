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
#include "colony.hpp"
#include "igui.hpp"

// config
#include "config/colony.rds.hpp"

// gs
#include "gs/unit-type.hpp"

// refl
#include "refl/to-str.hpp"

// Abseil
#include "absl/strings/str_replace.h"

using namespace std;

namespace rn {

namespace {

string fmt_construction( string const& name,
                         string const& needed ) {
  string parens = fmt::format( "({})", needed );
  return fmt::format( "{:<22} {}", name, needed );
}

string fmt_construction(
    string const&                name,
    ConstructionMaterials const& materials ) {
  string needed =
      fmt::format( "{:>3} hammers", materials.hammers );
  if( materials.tools != 0 )
    needed += fmt::format( ", {:>3} tools", materials.tools );
  return fmt_construction( name, needed );
}

void adjust_materials( Colony const&          colony,
                       ConstructionMaterials& materials ) {
  materials.hammers =
      std::max( materials.hammers - colony.hammers(), 0 );
  materials.tools = std::max(
      materials.tools - colony.commodities()[e_commodity::tools],
      0 );
}

string fmt_building( Colony const&     colony,
                     e_colony_building building ) {
  ConstructionMaterials materials =
      config_colony.materials_for_building[building];
  adjust_materials( colony, materials );
  return fmt_construction(
      construction_name(
          Construction::building{ .what = building } ),
      materials );
}

string fmt_unit( Colony const& colony, e_unit_type type ) {
  maybe<ConstructionMaterials> materials =
      config_colony.materials_for_unit[type];
  CHECK( materials.has_value() );
  adjust_materials( colony, *materials );
  return fmt_construction(
      construction_name( Construction::unit{ .type = type } ),
      *materials );
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

wait<> select_colony_construction( Colony& colony, IGui& gui ) {
  static string const kNoProductionKey = "none";
  ChoiceConfig config{ .msg = "Select One", .options = {} };
  config.options.push_back(
      ChoiceConfigOption{ .key          = kNoProductionKey,
                          .display_name = "(no production)" } );
  maybe<int> initial_selection;
  for( e_colony_building building :
       refl::enum_values<e_colony_building> ) {
    // TODO: need to check prerequisistes for these buildings.
    if( colony.buildings()[building] ) continue;
    config.options.push_back( ChoiceConfigOption{
        .key          = fmt::to_string( building ),
        .display_name = fmt_building( colony, building ) } );
    if( colony.construction().has_value() &&
        colony.construction() ==
            Construction_t{
                Construction::building{ .what = building } } )
      initial_selection = config.options.size() - 1;
  }
  for( e_unit_type type : refl::enum_values<e_unit_type> ) {
    if( !config_colony.materials_for_unit[type].has_value() )
      continue;
    if( unit_attr( type ).ship &&
        !colony.buildings()[e_colony_building::shipyard] )
      // Can't build ships without a shipyard.
      continue;
    // TODO: need to check prerequisistes for these units.
    config.options.push_back( ChoiceConfigOption{
        .key          = fmt::to_string( type ),
        .display_name = fmt_unit( colony, type ) } );
    if( colony.construction().has_value() &&
        colony.construction() ==
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
    colony.construction().reset();
    co_return;
  }

  for( e_colony_building building :
       refl::enum_values<e_colony_building> ) {
    if( what == fmt::to_string( building ) ) {
      colony.construction() =
          Construction::building{ .what = building };
      co_return;
    }
  }

  for( e_unit_type type : refl::enum_values<e_unit_type> ) {
    if( what == fmt::to_string( type ) ) {
      colony.construction() = Construction::unit{ .type = type };
      co_return;
    }
  }

  FATAL( "unknown building name {}.", *what );
}

} // namespace rn
