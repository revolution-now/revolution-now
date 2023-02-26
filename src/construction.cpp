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
#include "connectivity.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "ts.hpp"

// config
#include "config/colony.hpp"
#include "config/nation.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-type.hpp"

// refl
#include "refl/to-str.hpp"

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

maybe<construction_requirements> requirements_for_construction(
    Construction_t const& project ) {
  switch( project.to_enum() ) {
    case Construction::e::building:
      return config_colony.requirements_for_building
          [project.get<Construction::building>().what];
    case Construction::e::unit:
      return config_colony.requirements_for_unit
          [project.get<Construction::unit>().type];
  }
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

e_water_access colony_water_access( SSConst const& ss, TS& ts,
                                    Colony const& colony ) {
  e_water_access res = e_water_access::none;
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const moved = colony.location.moved( d );
    if( !ss.terrain.square_exists( moved ) ) continue;
    if( ss.terrain.square_at( moved ).surface !=
        e_surface::water )
      continue;
    res = std::max( res, e_water_access::yes );
    if( water_square_has_ocean_access( ts.connectivity, moved ) )
      res = e_water_access::coastal;
  }
  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
string construction_name( Construction_t const& construction ) {
  switch( construction.to_enum() ) {
    using e = Construction::e;
    case e::building: {
      auto& o = construction.get<Construction::building>();
      return config_colony.building_display_names[o.what];
    }
    case e::unit: {
      auto& o = construction.get<Construction::unit>();
      return unit_attr( o.type ).name;
    }
  }
}

wait<> select_colony_construction( SSConst const& ss, TS& ts,
                                   Colony& colony ) {
  static string const kNoProductionKey = "none";
  ChoiceConfig config{ .msg = "Select One", .options = {} };
  config.options.push_back(
      ChoiceConfigOption{ .key          = kNoProductionKey,
                          .display_name = "(no production)" } );
  maybe<int> initial_selection;
  int const  population = colony_population( colony );
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );
  e_water_access const water_access =
      colony_water_access( ss, ts, colony );
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
    if( requirements.requires_water > water_access ) continue;
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
    if( requirements.requires_water > water_access ) continue;
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
  maybe<string> const what =
      co_await ts.gui.optional_choice( config );

  if( what == nothing )
    // User cancelled; no change.
    co_return;

  if( what == kNoProductionKey ) {
    // User selected "(no production)".
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

maybe<RushConstruction> rush_construction_cost(
    SSConst const& ss, Colony const& colony ) {
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );
  UNWRAP_RETURN( project, colony.construction );
  UNWRAP_CHECK( total_requirements,
                requirements_for_construction( project ) );
  if( auto building = project.get_if<Construction::building>();
      building.has_value() && colony.buildings[building->what] )
    return nothing;
  auto needed_requirements = total_requirements;
  adjust_materials( colony, needed_requirements );
  int const hammers_cost =
      config_colony.rush_construction.cost_per_hammer *
      needed_requirements.hammers;
  int const tools_ask =
      market_price( player, e_commodity::tools ).ask;
  int const tools_cost =
      ( config_colony.rush_construction.base_cost_per_tool +
        config_colony.rush_construction
                .tools_ask_price_multiplier *
            tools_ask ) *
      needed_requirements.tools;
  bool const boycott_block =
      ( !config_colony.rush_construction
             .allow_rushing_tools_during_boycott &&
        player.old_world.market.commodities[e_commodity::tools]
            .boycott &&
        needed_requirements.tools > 0 );
  return RushConstruction{
      .project                  = project,
      .cost                     = hammers_cost + tools_cost,
      .total_hammers            = total_requirements.hammers,
      .total_tools              = total_requirements.tools,
      .needed_hammers           = needed_requirements.hammers,
      .needed_tools             = needed_requirements.tools,
      .blocked_by_tools_boycott = boycott_block };
}

wait<> rush_construction_prompt(
    Player& player, Colony& colony, IGui& gui,
    RushConstruction const& invoice ) {
  if( invoice.blocked_by_tools_boycott ) {
    co_await gui.message_box(
        "Rushing the construction of the [{}] requires "
        "acquiring [{} tools], but this is not allowed "
        "because tools are currently boycotted in {}.",
        construction_name( invoice.project ),
        invoice.needed_tools,
        config_nation.nations[player.nation].harbor_city_name );
    co_return;
  }

  string const msg =
      fmt::format( "Cost to complete [{}]: {}.  Treasury: {}.",
                   construction_name( invoice.project ),
                   invoice.cost, player.money );
  if( invoice.cost > player.money ) {
    // The player cannot afford it.
    co_await gui.message_box( msg );
    co_return;
  }
  // The player can afford it.

  maybe<ui::e_confirm> const answer =
      co_await gui.optional_yes_no(
          YesNoConfig{ .msg            = msg,
                       .yes_label      = "Complete it.",
                       .no_label       = "Never mind.",
                       .no_comes_first = true } );
  if( answer != ui::e_confirm::yes ) co_return;

  // Player has said to make it happen. As in the OG, we won't
  // actually build the thing, we will just provide the necessary
  // hammers and tools, and the project will finish at the start
  // of the next turn.
  player.money -= invoice.cost;
  CHECK_GE( player.money, 0 );
  if( colony.hammers < invoice.total_hammers )
    colony.hammers = invoice.total_hammers;
  if( colony.commodities[e_commodity::tools] <
      invoice.total_tools )
    colony.commodities[e_commodity::tools] = invoice.total_tools;
}

} // namespace rn
