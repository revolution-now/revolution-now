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
#include "imap-updater.hpp"
#include "market.hpp"
#include "player-mgr.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"

// config
#include "config/colony.hpp"
#include "config/nation.hpp"
#include "config/options.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

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
    string const& name,
    construction_requirements const& materials ) {
  string needed =
      fmt::format( "{:>3} hammers", materials.hammers );
  if( materials.tools != 0 )
    needed += fmt::format( ", {:>3} tools", materials.tools );
  return fmt_construction( name, needed );
}

void adjust_materials( Colony const& colony,
                       construction_requirements& materials ) {
  materials.hammers =
      std::max( materials.hammers - colony.hammers, 0 );
  materials.tools = std::max(
      materials.tools - colony.commodities[e_commodity::tools],
      0 );
}

maybe<construction_requirements> requirements_for_construction(
    Construction const& project ) {
  switch( project.to_enum() ) {
    case Construction::e::building:
      return config_colony.requirements_for_building
          [project.get<Construction::building>().what];
    case Construction::e::unit:
      return config_colony.requirements_for_unit
          [project.get<Construction::unit>().type];
  }
}

string fmt_building( Colony const& colony,
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
    if( water_square_has_ocean_access(
            ts.map_updater().connectivity(), moved ) )
      res = e_water_access::coastal;
  }
  return res;
}

config::colony::construction_requirements materials_needed(
    Construction const& construction ) {
  switch( construction.to_enum() ) {
    using e = Construction::e;
    case e::building: {
      auto& o = construction.get<Construction::building>();
      return config_colony.requirements_for_building[o.what];
    }
    case e::unit: {
      auto& o = construction.get<Construction::unit>();
      maybe<config::colony::construction_requirements> const&
          materials =
              config_colony.requirements_for_unit[o.type];
      CHECK( materials.has_value(),
             "a colony is constructing unit {}, but that unit "
             "is not buildable.",
             o.type );
      return *materials;
    }
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
string construction_name( Construction const& construction ) {
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
  int const population = colony_population( colony );
  UNWRAP_CHECK( player, ss.players.players[colony.player] );
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
            Construction{
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
            Construction{ Construction::unit{ .type = type } } )
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
  UNWRAP_CHECK( player, ss.players.players[colony.player] );
  UNWRAP_RETURN( project, colony.construction );
  UNWRAP_CHECK( total_requirements,
                requirements_for_construction( project ) );
  if( auto building = project.get_if<Construction::building>();
      building.has_value() && colony.buildings[building->what] )
    return nothing;
  auto needed_requirements = total_requirements;
  adjust_materials( colony, needed_requirements );
  int const zero_hammers_multiplier =
      ( colony.hammers > 0 ) ? 1 : 2;
  int const hammers_cost =
      config_colony.rush_construction.cost_per_hammer *
      needed_requirements.hammers * zero_hammers_multiplier;
  int const tools_ask =
      market_price( ss, player, e_commodity::tools ).ask;
  int const tools_cost =
      ( config_colony.rush_construction.base_cost_per_tool +
        tools_ask ) *
      needed_requirements.tools * zero_hammers_multiplier;
  bool const boycott_block =
      ( !config_colony.rush_construction
             .allow_rushing_tools_during_boycott &&
        old_world_state( ss, player.type )
            .market.commodities[e_commodity::tools]
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

bool wagon_train_limit_exceeded( SSConst const& ss,
                                 Player const& player ) {
  int const num_wagon_trains = [&] {
    int total = 0;
    for( auto const& [unit_id, p_state] : ss.units.euro_all() ) {
      Unit const& unit = ss.units.unit_for( unit_id );
      if( unit.player_type() != player.type ) continue;
      if( unit.type() != e_unit_type::wagon_train ) continue;
      ++total;
    }
    return total;
  }();
  int const max_allowed = [&] {
    switch( ss.settings.game_setup_options.customized_rules
                .wagon_train_limit_mode ) {
      using enum config::options::e_wagon_train_limit_mode;
      case classic:
        return int(
            ss.colonies.for_player( player.type ).size() );
      case population:
        return 1 +
               total_colonies_population( ss, player.type ) / 4;
      case none:
        return numeric_limits<int>::max();
    }
  }();
  return num_wagon_trains >= max_allowed;
}

maybe<ColonyBuilt> evolve_colony_construction(
    SS& ss, TS& ts, Player const& player, Colony& colony,
    vector<ColonyNotification>& notifications ) {
  maybe<ColonyBuilt> res;
  if( !colony.construction.has_value() ) return res;
  Construction const& construction = *colony.construction;

  // First check if it's a building that the colony already has.
  if( auto building =
          construction.get_if<Construction::building>();
      building.has_value() ) {
    if( colony.buildings[building->what] ) {
      // We already have the building, but we will only notify
      // the player (to minimize spam) if there are active car-
      // penter's building.
      if( !colony.indoor_jobs[e_indoor_job::hammers].empty() )
        notifications.emplace_back(
            ColonyNotification::construction_already_finished{
              .what = construction } );
      return res;
    }
  }

  auto const requirements = materials_needed( construction );

  if( colony_population( colony ) <
      requirements.minimum_population ) {
    notifications.emplace_back(
        ColonyNotification::construction_lacking_population{
          .what = construction,
          .required_population =
              requirements.minimum_population } );
    return res;
  }

  if( requirements.required_building.has_value() &&
      !colony_has_building_level(
          colony, *requirements.required_building ) ) {
    notifications.emplace_back(
        ColonyNotification::construction_lacking_building{
          .what = construction,
          .required_building =
              *requirements.required_building } );
    return res;
  }

  if( colony.hammers < requirements.hammers ) return res;

  int const have_tools = colony.commodities[e_commodity::tools];

  if( colony.commodities[e_commodity::tools] <
      requirements.tools ) {
    notifications.emplace_back(
        ColonyNotification::construction_missing_tools{
          .what       = construction,
          .have_tools = have_tools,
          .need_tools = requirements.tools } );
    return res;
  }

  // Check wagon train count limit.
  SWITCH( construction ) {
    CASE( building ) { break; }
    CASE( unit ) {
      if( unit.type != e_unit_type::wagon_train ) break;
      if( wagon_train_limit_exceeded( ss,
                                      as_const( player ) ) ) {
        notifications.emplace_back(
            ColonyNotification::wagon_train_limit_reached{} );
        return res;
      }
      break;
    }
  }

  // In the original game, when a construction finishes it resets
  // the hammers to zero, even if we've accumulated more than we
  // need (waiting for tools). If we didn't do this then it might
  // not give the player an incentive to get the tools in a
  // timely manner, since they would never lose hammers. That
  // said, it does appear to allow hammers to accumulate after a
  // construction (while we are waiting for the player to change
  // construction) and those hammers can then be reused on the
  // next construction (to some extent, depending on difficulty
  // level).
  colony.hammers = 0;
  colony.commodities[e_commodity::tools] -= requirements.tools;
  CHECK_GE( colony.commodities[e_commodity::tools], 0 );

  notifications.emplace_back(
      ColonyNotification::construction_complete{
        .what = construction } );

  switch( construction.to_enum() ) {
    using e = Construction::e;
    case e::building: {
      auto& o = construction.get<Construction::building>();
      add_colony_building( colony, o.what );
      res = ColonyBuilt::building{ .what = o.what };
      return res;
    }
    case e::unit: {
      auto& o = construction.get<Construction::unit>();
      // We need a real map updater here because e.g. theoreti-
      // cally we could construct a unit that has a two-square
      // sighting radius and that might cause new squares to be-
      // come visible, which would have to be rendered. That
      // said, in the original game, no unit that can be con-
      // structed in this manner has a sighting radius of more
      // than one.
      UnitId const unit_id = create_unit_on_map_non_interactive(
          ss, ts.map_updater(), player, o.type,
          colony.location );
      res = ColonyBuilt::unit{ .unit_id = unit_id };
      return res;
    }
  }
}

} // namespace rn
