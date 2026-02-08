/****************************************************************
**test-map.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-04.
*
* Description: Generates maps for testing and prints them to the
*              console.
*
*****************************************************************/
#include "test-map.hpp"

// Revolution Now
#include "ascii-map.hpp"
#include "connectivity.hpp"
#include "create-game.hpp"
#include "game-setup.hpp"
#include "iengine.hpp"
#include "irand.hpp"
#include "lua.hpp"
#include "map-updater.hpp"
#include "perlin-map.hpp"
#include "terrain-mgr.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/root.hpp"

// luapp
#include "luapp/ext-refl.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// rcl
#include "rcl/parse.hpp"

// cdr
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/logger.hpp"
#include "base/string.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <fstream>
#include <iostream>

namespace rn {

namespace {

using namespace std;

using ::base::lookup;
using ::base::ScopedTimer;
using ::base::str_replace_all;
using ::gfx::point;

void generate_single_map( IEngine& engine, SS& ss,
                          bool const reseed ) {
  lua::state st;
  lua_init( engine, st );
  SS ss;
  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  TerrainConnectivity connectivity;
  NonRenderingMapUpdater non_rendering_map_updater(
      ss, connectivity );
  st["IMapUpdater"] =
      static_cast<IMapUpdater&>( non_rendering_map_updater );
  IRand& rand = engine.rand();
  st["IRand"] = static_cast<IRand&>( rand );

  // Need to reseed the engine because the previous generated map
  // may have seeded it.
  engine.rand().reseed( rng::entropy::from_random_device() );

  // ------------------------------------------------------------
  // GameSetup
  // ------------------------------------------------------------
#if 0
  GameSetup setup;
  load_testing_game_setup( setup );
  if( reseed )
    setup.map.source.get_if<MapSource::generate_native>()
        ->setup.surface_generator.perlin_settings.seed =
        generate_perlin_seed(
            rand.generate_deterministic_seed() );
#elif 0
  (void)reseed;
  ClassicGameSetupParamsCommon const params{
    .difficulty  = e_difficulty::conquistador,
    .player      = e_nation::english,
    .player_name = "David" };
  GameSetup const setup =
      create_default_game_setup( rand, params );
#else
  (void)reseed;
  ClassicGameSetupParams const params{
    .common = { .difficulty  = e_difficulty::conquistador,
                .player      = e_nation::english,
                .player_name = "David" },
    .custom = { .land_mass   = e_land_mass::moderate,
                .land_form   = e_land_form::normal,
                .temperature = e_temperature::temperate,
                .climate     = e_climate::normal } };
  GameSetup const setup =
      create_classic_customized_game_setup( rand, params );
#endif

  // ------------------------------------------------------------
  // Generate map.
  // ------------------------------------------------------------
  CHECK_HAS_VALUE(
      create_game_from_setup( ss, rand, st, setup ) );
}

/****************************************************************
** Stats collectors.
*****************************************************************/
struct IGameStatsCollector {
  virtual ~IGameStatsCollector()            = default;
  virtual void collect( SSConst const& ss ) = 0;
  virtual void write() const                = 0;
};

struct LandDensityStats : IGameStatsCollector {
  void collect( SSConst const& ss ) override {
    ++maps_total;
    on_all_tiles(
        ss, [&]( point const tile, MapSquare const& square ) {
          land_count_x[tile.x];
          land_count_y[tile.y];
          if( square.surface == e_surface::water ) return;
          ++land_total;
          ++land_count_x[tile.x];
          ++land_count_y[tile.y];
        } );
  }

  void write() const override {
    fs::path const generated =
        "tools/auto-measure/auto-map-gen/land-density/generated";
    ofstream csv( generated / "land-density.csv" );
    ofstream gnu( generated / "land-density.gnuplot" );
    string const GNUPLOT_FILE_TEMPLATE = R"gnuplot(
      #!/usr/bin/env -S gnuplot -p
      set title "{{TITLE}} ({{MODE}} [{{COUNT}}])"
      set datafile separator ","
      set key outside right
      set grid
      set xlabel "X or Y coordinate"
      set ylabel "density"

      # Use the first row as column headers for titles.
      set key autotitle columnhead

      set yrange [0:1.0]
      set xrange [{{XRANGE}}]

      plot for [col=2:*] "{{CSV_STEM}}" using 1:col with lines lw 2
    )gnuplot";
    string const gnuplot_body = base::trim( str_replace_all(
        GNUPLOT_FILE_TEMPLATE,
        {
          { "{{TITLE}}", "Spatial Land Density (generated)" },
          { "{{CSV_STEM}}", "generated.csv" },
          { "{{MODE}}", "mode" },
          { "{{COUNT}}", to_string( maps_total ) },
          { "{{XRANGE}}", "0:1.0" },
        } ) );
    gnu << gnuplot_body;

    csv << format( "coordinate,x,y,overall\n" );
    double const density =
        land_total / ( 56.0 * 70 * maps_total );
    for( double p = 0; p < 1; p += .001 ) {
      csv << p;
      int const x = int( floor( p * 56 ) );
      int const y = int( floor( p * 70 ) );
      CHECK( x >= 0 );
      CHECK( y >= 0 );
      CHECK( x < 56 );
      CHECK( y < 70 );
      UNWRAP_CHECK_T( int const count_x,
                      lookup( land_count_x, x ) );
      UNWRAP_CHECK_T( int const count_y,
                      lookup( land_count_y, y ) );
      double const density_x = count_x / ( 70.0 * maps_total );
      double const density_y = count_y / ( 56.0 * maps_total );
      csv << format( ",{},{},{}\n", density_x, density_y,
                     density );
    }
  }

  map<int, int> land_count_x;
  map<int, int> land_count_y;
  int land_total = 0;
  int maps_total = 0;
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void load_testing_game_setup( GameSetup& setup ) {
  UNWRAP_CHECK_T( rcl::doc const& doc,
                  rcl::parse_file( "keys/game-exchange-key.rcl",
                                   rcl::ProcessingOptions{} ) );
  cdr::converter::options const options{
    .allow_unrecognized_fields        = true,
    .default_construct_missing_fields = false,
  };
  UNWRAP_CHECK_T( setup,
                  cdr::run_conversion_from_canonical<GameSetup>(
                      doc.top_val(), options ) );
}

void testing_map_gen( IEngine& engine ) {
  SS ss;
  generate_single_map( engine, ss, /*reseed=*/true );
  print_ascii_map( ss.terrain.real_terrain(), cout );
}

void testing_map_gen_stats( IEngine& engine ) {
  int const n = 2000;
  ScopedTimer const timer( format( "generate {} maps", n ) );
  LandDensityStats stats;
  for( int i = 0; i < n; ++i ) {
    lg.info( "generating map {}...", i );
    SS ss;
    generate_single_map( engine, ss, /*reseed=*/true );
    // print_ascii_map( ss.terrain.real_terrain(), cout );
    stats.collect( ss );
  }
  stats.write();
}

} // namespace rn
