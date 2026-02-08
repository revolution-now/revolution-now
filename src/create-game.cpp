/****************************************************************
**create-game.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Handles the setup of a new game.
*
*****************************************************************/
#include "create-game.hpp"

// Revolution Now
#include "classic-sav.hpp"
#include "co-wait.hpp"
#include "irand.hpp"
#include "map-gen.hpp"
#include "perlin-map.hpp"
#include "terrain-mgr.hpp"

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

// gfx
#include "gfx/iter.hpp"

// rcl
#include "rcl/to.hpp"

// cdr
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// rand
#include "rand/random.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::ScopedTimer;
using ::base::valid;
using ::base::valid_or;
using ::gfx::Matrix;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::gfx::size;
using ::refl::enum_values;

[[nodiscard]] bool has_bonuses( RealTerrain const& terrain ) {
  for( point const p : rect_iterator( terrain.map.rect() ) ) {
    MapSquare const& square = terrain.map[p];
    if( square.ground_resource.has_value() ) return true;
    if( square.forest_resource.has_value() ) return true;
    if( square.lost_city_rumor ) return true;
  }
  return false;
}

void convert_islands_to_mountains( RealTerrain& out ) {
  for( point const p : rect_iterator( out.map.rect() ) )
    if( is_island( as_const( out ), p ) )
      out.map[p].overlay = e_land_overlay::mountains;
}

valid_or<string> generate_land_perlin(
    GeneratedMapSetup const& setup, IRand& rand,
    RealTerrain& real_terrain ) {
  size const sz                 = setup.size;
  auto const& surface_generator = setup.surface_generator;

  // Land form generator.
  {
    auto const& perlin_settings =
        surface_generator.perlin_settings;
    Matrix<e_surface> surface;
    TIMED_CALL( land_gen_perlin, perlin_settings,
                surface_generator.target_density, setup.size,
                surface );
    for( point const p : rect_iterator( surface.rect() ) )
      real_terrain.map[p].surface = surface[p];
    // FIXME: temporary
    for( point const p : rect_iterator( surface.rect() ) )
      real_terrain.map[p].ground = e_ground_terrain::grassland;
  }

  bool const arctic_enabled =
      setup.surface_generator.arctic.enabled;

  // Arctic. Should be done before exclusion.
  if( arctic_enabled ) {
    rand.reseed( setup.surface_generator.arctic.seed );
    double const actual_density =
        place_arctic( real_terrain, rand,
                      setup.surface_generator.arctic.density );
    lg.info( "arctic density: {:.3}%", actual_density * 100 );
  }

  // Hard buffer exclusion.
  auto const apply_exclusion = [&] {
    using enum e_surface;
    rect const land_zone = compute_land_zone( sz );
    lg.info(
        "land_zone.left={}, land_zone.right={}, "
        "land_zone.top={}, land_zone.bottom={}",
        land_zone.left(), sz.w - land_zone.right(),
        land_zone.top(), sz.h - land_zone.bottom() );
    auto& m = real_terrain.map;
    for( point const p : rect_iterator( m.rect() ) ) {
      if( p.is_inside( land_zone ) ) continue;
      if( arctic_enabled ) {
        // The idea here is that on the arctic rows (if arctic is
        // enabled) we allow land on all tiles except for the
        // four corners of the map.
        if( p.y == 0 || p.y == sz.h - 1 )
          if( p.x > 0 && p.x < sz.w - 1 ) //
            continue;
      }
      m[p].surface = water;
    }
  };

  // Run this the first time before the sanitization because re-
  // moving some land tiles might create sanitization violations
  // such as crosses.
  apply_exclusion();

  // Sanitization.
  {
    auto const& sanitization = surface_generator.sanitization;
    if( sanitization.remove_crosses )
      TIMED_CALL( remove_crosses, real_terrain );
    if( sanitization.remove_islands )
      TIMED_CALL( remove_islands, real_terrain );
  }

  // Now run it again just in case e.g. the cross removal ends up
  // putting a land tile in the exclusion zone.
  apply_exclusion();

  // TODO: it is possible that we might still end up with islands
  // here, and so we should probably do a pass later to put moun-
  // tains on them.

  return valid;
}

valid_or<string> generate_map_native_impl(
    GeneratedMapSetup const& setup, RootState& root,
    RealTerrain& real_terrain, IRand& rand ) {
  CHECK( setup.size.area() > 0, "map has zero tiles" );

  // Surface.
  GOOD_OR_RETURN(
      generate_land_perlin( setup, rand, real_terrain ) );

  generate_proto_tiles( root.zzz_terrain );

  return valid;
}

valid_or<string> generate_map_lua_impl(
    GeneratedMapSetup const& setup, lua::state& st ) {
  lua::table M = st["map_gen"].as<lua::table>();

  // Proto squares.
  // ------------------------------------------------------------
  M["generate_proto_squares"]();

  // Land generation.
  // ------------------------------------------------------------
  lua::table options = st.table.create();
  options["brush"]   = "mixed";
  options["land_density"] =
      setup.surface_generator.target_density;
  options["remove_crosses"] =
      ( setup.surface_generator.sanitization.remove_crosses );
  options["min_map_height_for_arctic"] = 10;

  options["river_density"]    = setup.rivers.density;
  options["hills_density"]    = setup.hills.density;
  options["mountain_density"] = setup.mountains.density;
  options["hills_range_probability"] =
      setup.hills.range_probability;
  options["mountain_range_probability"] =
      setup.mountains.range_probability;
  options["major_river_fraction"] =
      setup.rivers.major_river_fraction;
  options["forest_density"] = setup.forest.density;
  options["arctic_tile_density"] =
      setup.surface_generator.arctic.density;

  // Assign land vs. water.
  M["generate_land"]( options );
  if( setup.surface_generator.arctic.enabled )
    M["create_arctic"]( setup.surface_generator.arctic.density );
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

  // Pacific Ocean.
  // ------------------------------------------------------------
  M["create_pacific_ocean"]();

  return valid;
}

valid_or<string> generate_map_native(
    GeneratedMapSetup const& setup, RootState& root,
    IRand& rand ) {
  CHECK( setup.size.area() > 0, "map has zero tiles" );

  auto modifier =
      [&]( RealTerrain& real_terrain ) -> valid_or<string> {
    GOOD_OR_RETURN( generate_map_native_impl(
        setup, root, real_terrain, rand ) );
    return valid;
  };

  valid_or<string> res = valid;
  root.zzz_terrain.modify_entire_map(
      [&]( RealTerrain& real_terrain ) {
        real_terrain.map = Matrix<MapSquare>( setup.size );
        res              = modifier( real_terrain );
      } );
  return res;
}

valid_or<string> generate_map_lua(
    GeneratedMapSetup const& setup, lua::state& st,
    TerrainState& terrain ) {
  CHECK( setup.size.area() > 0, "map has zero tiles" );

  auto modifier = [&]( RealTerrain& ) {
    return generate_map_lua_impl( setup, st );
  };

  valid_or<string> res = valid;
  terrain.modify_entire_map( [&]( RealTerrain& real_terrain ) {
    real_terrain.map = Matrix<MapSquare>( setup.size );
    res              = modifier( real_terrain );
  } );
  return res;
}

valid_or<string> load_map_from_file(
    MapSource::load_from_file const& load_from_file,
    lua::state& lua, RootState& root ) {
  auto const load_terrain = [&]( RealTerrain&& src ) {
    root.zzz_terrain.modify_entire_map(
        [&]( RealTerrain& out ) { out = std::move( src ); } );
  };

  // Load the map.
  switch( load_from_file.type.epoch ) {
    case e_map_file_epoch::classic: {
      switch( load_from_file.type.format ) {
        case e_map_file_format::binary: {
          RealTerrain loaded;
          GOOD_OR_RETURN( load_classic_binary_map_file(
              load_from_file.path, loaded ) );
          load_terrain( std::move( loaded ) );
          break;
        }
        case e_map_file_format::json:
          return "classic json map format not yet "
                 "supported.";
        case e_map_file_format::rcl:
          return "classic rcl map format not yet supported.";
      }
      break;
    }
    case e_map_file_epoch::modern: {
      switch( load_from_file.type.format ) {
        case e_map_file_format::rcl:
          return "modern rcl map format not yet supported.";
        case e_map_file_format::binary:
          return "modern binary map format not yet "
                 "supported.";
        case e_map_file_format::json:
          return "modern json map format not yet supported.";
      }
      break;
    }
  }

  // Set proto tiles.
  generate_proto_tiles( root.zzz_terrain );

  if( load_from_file.convert_islands_to_mountains )
    root.zzz_terrain.modify_entire_map( [&]( RealTerrain& out ) {
      convert_islands_to_mountains( out );
    } );

  // Generate prime resources if needed.
  //
  // Depending on which map type we've loaded above, there may or
  // may not already be bonuses distributed there. For the
  // classic maps there will not be, and for the modern maps
  // there may be, though there may not be. So in either case we
  // will search the map to see if there are any bonuses and if
  // so we will leave them as they are, otherwise we will redis-
  // tribute them. Classic maps do not have prime resources and
  // LCRs builtin, thus we need to distribute them here.
  //
  // NOTE: we need to do this after mountainizing islands because
  // putting mountains on a tile affects which prime resource it
  // gets. Actually this is irrelevant in a sense because since
  // it has mountains on it, the tile will not be eligible as a
  // colony site and therefore no colony will be able to mine
  // that tile. That said, it will look incorrect if we e.g. put
  // a cotton resource on a mountain tile just from a visual
  // standpoint.
  if( !has_bonuses( root.zzz_terrain.real_terrain() ) ) {
    rng::random r( load_from_file.bonuses.seed );
    auto const distribute_bonuses =
        lua["map_gen"]["distribute_bonuses"];
    distribute_bonuses( r.uniform_int( 0, 255 ) );
  }

  return valid;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
// NOTE: we visit each section in reverse order that it appears
// in the struct, since some of the earlier ones depend on the
// later ones.
valid_or<string> create_game_from_setup(
    SS& ss, IRand& rand, lua::state& lua,
    GameSetup const& setup ) {
  RootState& root = ss.root;

  root = {}; // Just in case.

#if 0
  lg.info( "creating game from:\n{}", rcl::to_rcl( setup ) );
  // lg.info( "creating game from:\n{}", rcl::to_json( setup ) );
#endif

  // SettingsState state.
  // ------------------------------------------------------------
  lg.info( "game setup: setting settings state..." );
#if 1
  lua::table settings_options    = lua.table.create();
  settings_options["difficulty"] = setup.settings.difficulty;
  lua["new_game"]["create_settings_state"]( settings_options );
#endif

  // Rules.
  // ------------------------------------------------------------
  lg.info( "game setup: setting rules..." );
  root.settings.game_setup_options.customized_rules =
      setup.settings.rules;

  // Map.
  // ------------------------------------------------------------
  lg.info( "game setup: generating map..." );
  SWITCH( setup.map.source ) {
    CASE( load_from_file ) {
      GOOD_OR_RETURN(
          load_map_from_file( load_from_file, lua, root ) );
      break;
    }
    CASE( generate_native ) {
      ScopedTimer const timer( "total terrain generation time" );
      GOOD_OR_RETURN( generate_map_native( generate_native.setup,
                                           root, rand ) );
      break;
    }
    CASE( generate_lua ) {
      ScopedTimer const timer( "total terrain generation time" );
      GOOD_OR_RETURN(
          generate_map_lua( generate_lua.setup, lua,
                            ss.mutable_terrain_use_with_care ) );
      break;
    }
  }

#if 1
  // Natives.
  // ------------------------------------------------------------
  lg.info( "game setup: generating natives..." );
  lua::table natives_options            = lua.table.create();
  natives_options["dwelling_frequency"] = 0.14; // TODO
  lua::table native_tribes = lua::table::create_or_get(
      natives_options["native_tribes"] );
  for( int i = 1; auto const& [tribe_type, tribe_setup] :
                  setup.natives.tribes ) {
    if( !tribe_setup.has_value() ) continue;
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
    player["control"] = base::to_str( nation_setup->control );
    players_unordered[nation] = player;
    ordered_players[i++]      = player;
  }
  lua["new_game"]["create_players"]( player_options );

  // Create turn state.
  // ------------------------------------------------------------
  lg.info( "game setup: generating turn state..." );
  ss.turn.time_point.year = setup.settings.starting_year;
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
#endif

  return valid;
}

} // namespace rn
