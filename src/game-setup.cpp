/****************************************************************
**game-setup.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Handles the setup of a new game.
*
*****************************************************************/
#include "game-setup.hpp"

// Revolution Now
#include "co-wait.hpp"

// config
#include "config/turn.rds.hpp"

// ss
#include "error.hpp"
#include "ss/ref.hpp"
#include "ss/root.hpp"

// rds
#include "rds/switch-macro.hpp"

// luapp
#include "luapp/enum.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::NoDiscard;

[[nodiscard]] bool generated_terrain_setup_perlin(
    GeneratedTerrainSetup const&,
    LandGeneratorAlgorithm::perlin const& ) {
  NOT_IMPLEMENTED;
}

[[nodiscard]] bool generated_terrain_setup_seeded_organic(
    lua::state& lua, GeneratedTerrainSetup const& setup,
    LandGeneratorAlgorithm::seeded_organic const&
        seeded_organic_setup ) {
  lua::table M = lua["map_gen"].as<lua::table>();

  // Proto squares.
  // ------------------------------------------------------------
  M["generate_proto_squares"]();

  // Land generation.
  // ------------------------------------------------------------
  lua::table options      = lua.table.create();
  options["brush"]        = seeded_organic_setup.brush;
  options["land_density"] = setup.land_generator.target_density;
  options["remove_Xs"] =
      ( setup.land_generator.remove_Xs_probability > 0.0 );
  options["remove_Xs_probability"] =
      setup.land_generator.remove_Xs_probability;
  options["min_map_height_for_arctic"] = 10;

  options["river_density"]    = setup.river_density;
  options["hills_density"]    = setup.hills_density;
  options["mountain_density"] = setup.mountain_density;
  options["hills_range_probability"] =
      setup.hills_range_probability;
  options["mountain_range_probability"] =
      setup.mountain_range_probability;
  options["major_river_fraction"] = setup.major_river_fraction;
  options["forest_density"]       = setup.forest_density;
  options["arctic_tile_density"]  = setup.arctic_tile_density;

  // Assign land vs. water.
  M["generate_land"]( options );
  if( setup.add_arctic_tiles ) M["create_arctic"]( options );
  M["assign_dry_ground_types"]();
  // We need to have already created the rivers before this.
  M["assign_wet_ground_types"]();
  // These need to be done before the rivers since rivers don't
  // seem to flow on hills/mountains in the OG.
  M["create_hills"]( options );
  M["create_mountains"]( options );
  M["create_rivers"]( options );
  M["forest_cover"]( options );
  M["distribute_bonuses"]();

  // Sea lane.
  // ------------------------------------------------------------
  M["create_sea_lanes"]();

  // Arctic.
  // ------------------------------------------------------------
  M["create_pacific_ocean"]();

  return true;
}

wait_bool generated_terrain_setup(
    TerrainState& terrain, IRand&, lua::state& lua,
    GeneratedTerrainSetup const& setup ) {
  CHECK( setup.size.area() > 0, "map has zero tiles" );
  terrain.modify_entire_map( [&]( RealTerrain& real_terrain ) {
    real_terrain.map = gfx::Matrix<MapSquare>( setup.size );
  } );
  SWITCH( setup.land_generator.generator_algo ) {
    CASE( perlin ) {
      co_return generated_terrain_setup_perlin( setup, perlin );
    }
    CASE( seeded_organic ) {
      co_return generated_terrain_setup_seeded_organic(
          lua, setup, seeded_organic );
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
// NOTE: we visit each section in reverse order that it appears
// in the struct, since some of the earlier ones depend on the
// later ones.
wait_bool create_game_from_setup( SS& ss, IRand& rand,
                                  lua::state& lua,
                                  GameSetup const& setup ) {
  RootState& root = ss.root;

  // SettingsState state.
  // ------------------------------------------------------------
  lg.info( "game setup: setting settings state..." );
  lua::table settings_options = lua.table.create();
  settings_options["difficulty"] =
      setup.settings_state.difficulty;
  lua["new_game"]["create_settings_state"]( settings_options );

  // Rules.
  // ------------------------------------------------------------
  lg.info( "game setup: setting rules..." );
  root.settings.game_setup_options.customized_rules =
      setup.rules.values;

  // Map.
  // ------------------------------------------------------------
  lg.info( "game setup: generating map..." );
  SWITCH( setup.map ) {
    CASE( load_from_file ) { NOT_IMPLEMENTED; }
    CASE( generate ) {
      if( !co_await generated_terrain_setup(
              ss.mutable_terrain_use_with_care, rand, lua,
              generate.terrain ) )
        co_return false;
      break;
    }
  }

  // Natives.
  // ------------------------------------------------------------
  lg.info( "game setup: generating natives..." );
  lua::table natives_options = lua.table.create();
  natives_options["dwelling_frequency"] =
      setup.natives.dwelling_frequency;
  lua::table native_tribes = lua::table::create_or_get(
      natives_options["native_tribes"] );
  for( int i = 1; auto const& [tribe_type, tribe_setup] :
                  setup.natives.tribes ) {
    if( tribe_setup.disabled ) continue;
    native_tribes[i++] = tribe_type;
  }
  natives_options["native_tribes"] = native_tribes;
  lua["map_gen"]["create_indian_villages"]( natives_options );

  // Nations/Players.
  // ------------------------------------------------------------
  lg.info( "game setup: generating nations..." );
  lua::table player_options         = lua.table.create();
  lua::table ordered_players        = lua.table.create();
  lua::table players_unordered      = lua.table.create();
  player_options["ordered_players"] = ordered_players;
  player_options["players"]         = players_unordered;
  for( int i = 1; auto const& [nation, nation_setup] :
                  setup.nations.nations ) {
    if( !nation_setup.has_value() ) continue;
    lua::table player = lua.table.create();
    player["nation"]  = nation;
    player["control"] = nation_setup->human ? "human" : "ai";
    players_unordered[nation] = player;
    ordered_players[i++]      = player;
  }
  lua["new_game"]["create_players"]( player_options );

  // Create turn state.
  // ------------------------------------------------------------
  lg.info( "game setup: generating turn state..." );
  ss.turn.time_point.year = setup.turns.starting_year;
  ss.turn.time_point.season =
      config_turn.game_start.starting_season;

  // Initial units.
  // ------------------------------------------------------------
  lg.info( "game setup: generating initial ships..." );
  lua["new_game"]["create_units_state"]( player_options );

  // Map view state.
  // ------------------------------------------------------------
  lg.info( "game setup: generating map view state..." );
  lua["new_game"]["create_mapview_state"]( player_options );

  co_return true;
}

} // namespace rn
