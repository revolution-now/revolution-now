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

// sav
#include "sav/binary.hpp"
#include "sav/bridge.hpp"
#include "sav/map-file.hpp"

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

using ::base::function_ref;
using ::base::lookup;
using ::base::ScopedTimer;
using ::base::str_replace_all;
using ::gfx::point;
using ::gfx::size;
using ::refl::enum_map;
using ::refl::enum_values;

/****************************************************************
** Helpers.
*****************************************************************/
string_view mode_name( e_temperature const t,
                       e_climate const c ) {
  switch( t ) {
    case e_temperature::cool:
      switch( c ) {
        case e_climate::arid:
          return "bbtt";
        case e_climate::normal:
          return "bbtm";
        case e_climate::wet:
          return "bbtb";
      }
      break;
    case e_temperature::temperate:
      switch( c ) {
        case e_climate::arid:
          return "bbmt";
        case e_climate::normal:
          return "bbmm";
        case e_climate::wet:
          return "bbmb";
      }
      break;
    case e_temperature::warm:
      switch( c ) {
        case e_climate::arid:
          return "bbbt";
        case e_climate::normal:
          return "bbbm";
        case e_climate::wet:
          return "bbbb";
      }
      break;
  }
}

/****************************************************************
** Game/Map Generators.
*****************************************************************/
void generate_single_map_impl(
    IEngine& engine, SS& ss,
    function_ref<void( IRand&, GameSetup& ) const> const fn ) {
  TerrainConnectivity connectivity;
  NonRenderingMapUpdater non_rendering_map_updater(
      ss, connectivity );
  IRand& rand = engine.rand();

  lua::state st;
#if 0
  lua_init( engine, st );
  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  st["IMapUpdater"] =
      static_cast<IMapUpdater&>( non_rendering_map_updater );
  st["IRand"] = static_cast<IRand&>( rand );
#endif

  // Need to reseed the engine because the previous generated map
  // may have seeded it.
  rand.reseed( rng::entropy::from_random_device() );

  // ------------------------------------------------------------
  // GameSetup
  // ------------------------------------------------------------
  GameSetup setup;
  fn( rand, setup );

  // ------------------------------------------------------------
  // Generate map.
  // ------------------------------------------------------------
  CHECK_HAS_VALUE(
      create_game_from_setup( ss, rand, st, setup ) );
}

void generate_single_map_key( IEngine& engine, SS& ss,
                              bool const reseed ) {
  auto const fn = [&]( IRand& rand, GameSetup& setup ) {
    load_testing_game_setup( setup );
    UNWRAP_CHECK_T(
        auto& generate_native,
        setup.map.source.get_if<MapSource::generate_native>() );
    auto& native = generate_native.setup;
    if( reseed ) {
      auto const S = [&] {
        return rand.generate_deterministic_seed();
      };
      auto& surf_gen = native.surface_generator;

      surf_gen.perlin_settings.seed =
          generate_perlin_seed( S() );
      surf_gen.arctic.seed = S();
      native.biomes.seed   = S();
    }
  };
  generate_single_map_impl( engine, ss, fn );
}

[[maybe_unused]] void generate_single_map_new( IEngine& engine,
                                               SS& ss ) {
  auto const fn = [&]( IRand& rand, GameSetup& setup ) {
    ClassicGameSetupParamsCommon const params{
      .difficulty  = e_difficulty::conquistador,
      .player      = e_nation::english,
      .player_name = "David" };
    setup = create_default_game_setup( rand, params );
  };
  generate_single_map_impl( engine, ss, fn );
}

[[maybe_unused]] void generate_single_map_custom(
    IEngine& engine, SS& ss,
    ClassicGameSetupParamsCustom const& custom ) {
  auto const fn = [&]( IRand& rand, GameSetup& setup ) {
    ClassicGameSetupParams const params{
      .common = { .difficulty  = e_difficulty::conquistador,
                  .player      = e_nation::english,
                  .player_name = "Conquistador David" },
      .custom = custom };
    setup = create_classic_customized_game_setup( rand, params );
  };
  generate_single_map_impl( engine, ss, fn );
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
    map_sz_ = ss.terrain.world_size_tiles();
    ++maps_total_;
    on_all_tiles(
        ss, [&]( point const tile, MapSquare const& square ) {
          land_count_x_[tile.x];
          land_count_y_[tile.y];
          if( square.surface == e_surface::water ) return;
          bool const arctic_row =
              tile.y == 0 || tile.y == map_sz_.h - 1;
          if( !arctic_row ) {
            ++land_total_;
            ++land_count_x_[tile.x];
            ++land_count_y_[tile.y];
          }
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
          { "{{CSV_STEM}}", "land-density.csv" },
          { "{{MODE}}", "c++" },
          { "{{COUNT}}", to_string( maps_total_ ) },
          { "{{XRANGE}}", "0:1.0" },
        } ) );
    gnu << gnuplot_body;

    csv << format( "coordinate,x,y,overall\n" );
    // NOTE: subtract two because we did not include the arctic
    // rows when collecting land.
    double const density =
        land_total_ / ( double( map_sz_.w ) * ( map_sz_.h - 2 ) *
                        maps_total_ );
    for( double p = 0; p < 1; p += .001 ) {
      csv << p;
      int const x = int( floor( p * map_sz_.w ) );
      int const y = int( floor( p * map_sz_.h ) );
      CHECK( x >= 0 );
      CHECK( y >= 0 );
      CHECK( x < map_sz_.w );
      CHECK( y < map_sz_.h );
      UNWRAP_CHECK_T( int const count_x,
                      lookup( land_count_x_, x ) );
      UNWRAP_CHECK_T( int const count_y,
                      lookup( land_count_y_, y ) );
      double const density_x =
          count_x / ( double( map_sz_.h - 2 ) * maps_total_ );
      double const density_y =
          count_y / ( double( map_sz_.w ) * maps_total_ );
      csv << format( ",{},{},{}\n", density_x, density_y,
                     density );
    }
  }

 private:
  size map_sz_ = {};
  map<int, int> land_count_x_;
  map<int, int> land_count_y_;
  int land_total_ = 0;
  int maps_total_ = 0;
};

struct BiomeDensityStats : IGameStatsCollector {
  static array<e_ground_terrain, 9> constexpr kGroundTypes = {
    e_ground_terrain::savannah, e_ground_terrain::grassland,
    e_ground_terrain::tundra,   e_ground_terrain::plains,
    e_ground_terrain::prairie,  e_ground_terrain::desert,
    e_ground_terrain::swamp,    e_ground_terrain::marsh,
    e_ground_terrain::arctic,
  };

  BiomeDensityStats( string const& stem ) : stem_( stem ) {
    CHECK( !stem_.empty() );
  }

  void collect( SSConst const& ss ) override {
    map_sz_ = ss.terrain.world_size_tiles();
    CHECK(
        map_sz_.h == 70,
        "this stats collector requires the standard map size." );
    ++maps_total_;
    on_all_tiles(
        ss, [&]( point const tile, MapSquare const& square ) {
          if( square.surface == e_surface::water ) return;
          ++land_total_;
          ++land_count_y_[tile.y + 1];
          ++biome_count_y_[tile.y + 1][square.ground];
        } );
  }

  void write() const override {
    fs::path const generated =
        "tools/auto-measure/auto-map-gen/biomes/generated";
    ofstream csv( generated / format( "{}.csv", stem_ ) );
    ofstream gnu( generated / format( "{}.gnuplot", stem_ ) );
    CHECK( csv.good() );
    CHECK( gnu.good() );
    string const GNUPLOT_FILE_TEMPLATE = R"gnuplot(
      #!/usr/bin/env -S gnuplot -p
      set title "{{TITLE}} ({{MODE}} [{{COUNT}}])"
      set datafile separator ","
      set key outside right
      set grid
      set xlabel "Map Row (Y)"
      set ylabel "Density"

      # Use the first row as column headers for titles.
      set key autotitle columnhead

      set yrange [0:0.7]
      set xrange [{{XRANGE}}]

      plot for [col=2:*] "{{CSV_STEM}}.csv" using 1:col with lines lw 2
    )gnuplot";
    string const gnuplot_body = base::trim( str_replace_all(
        GNUPLOT_FILE_TEMPLATE,
        {
          { "{{TITLE}}", "Biome Density (generated)" },
          { "{{CSV_STEM}}", stem_ },
          { "{{MODE}}", stem_ },
          { "{{COUNT}}", to_string( maps_total_ ) },
          { "{{XRANGE}}", "0:70" },
        } ) );
    gnu << gnuplot_body;

    // Header.
    csv << format( "y" );
    for( auto const gt : kGroundTypes )
      csv << ',' << base::to_str( gt );
    csv << '\n';
    for( int y = 0; y < map_sz_.h; ++y ) {
      csv << y + 1;
      for( auto const gt : kGroundTypes ) {
        double value = 0.0;
        {
          auto const biome_count_y_row =
              lookup( biome_count_y_, y + 1 );
          if( !biome_count_y_row.has_value() ) goto skip;
          auto const land_count_y_row =
              lookup( land_count_y_, y + 1 );
          if( !land_count_y_row.has_value() ) goto skip;
          auto const biome_count_y_row_gt =
              lookup( *biome_count_y_row, gt );
          if( !biome_count_y_row_gt.has_value() ) goto skip;
          value = double( *biome_count_y_row_gt ) /
                  *land_count_y_row;
        }
      skip:
        csv << ',' << value;
      }
      csv << '\n';
    }
  }

 private:
  string const stem_;
  size map_sz_ = {};
  map<int /*y+1*/, map<e_ground_terrain, int /*count*/>>
      biome_count_y_;
  map<int /*y+1*/, int> land_count_y_;
  int land_total_ = 0;
  int maps_total_ = 0;
};

struct BiomeAdjacencyStats : IGameStatsCollector {
  static array<e_ground_terrain, 9> constexpr kGroundTypes = {
    e_ground_terrain::savannah, e_ground_terrain::grassland,
    e_ground_terrain::tundra,   e_ground_terrain::plains,
    e_ground_terrain::prairie,  e_ground_terrain::desert,
    e_ground_terrain::swamp,    e_ground_terrain::marsh,
    e_ground_terrain::arctic,
  };

  void collect( SSConst const& ss ) override {
    map_sz_ = ss.terrain.world_size_tiles();
    ++maps_total_;
    on_all_tiles(
        ss, [&]( point const tile, MapSquare const& center ) {
          if( center.surface == e_surface::water ) return;
          ++land_total_;
          ++terrain_total_[center.ground];
          ++land_per_row_[tile.y];
          ++ground_per_row_[center.ground][tile.y];
          on_surrounding(
              ss, tile,
              [&]( point const, MapSquare const& adjacent ) {
                if( adjacent.surface == e_surface::water )
                  return;
                ++surrounding_per_row_[center.ground][tile.y];
                if( adjacent.ground == center.ground )
                  ++adjacency_[adjacent.ground];
              } );
        } );
  }

  void write() const override {
    fmt::println( "maps_total: {}", maps_total_ );
    fmt::println( "land_total: {}", land_total_ );
    fmt::println( "" );
    enum_map<e_ground_terrain, double> relative_adjacencies;
    for( e_ground_terrain const gt : kGroundTypes ) {
      double adjacency_baseline = 0;
      for( auto const& [y, count] : ground_per_row_[gt] ) {
        UNWRAP_CONTINUE( int const land_on_row,
                         lookup( land_per_row_, y ) );
        UNWRAP_CONTINUE( int const surrounding_on_row,
                         lookup( surrounding_per_row_[gt], y ) );
        double const density_at_row =
            double( count ) / land_on_row;
        double const adjacency_baseline_at_row =
            density_at_row * surrounding_on_row;
        adjacency_baseline += adjacency_baseline_at_row;
      }
      double const adjacency_avg =
          double( adjacency_[gt] ) / terrain_total_[gt];
      double const result = adjacency_[gt] / adjacency_baseline;
      relative_adjacencies[gt] = result;
      fmt::println( "{}: {:.3f} | {}/{:.3f} ==> {:.3f}", gt,
                    adjacency_avg, adjacency_[gt],
                    adjacency_baseline, result );
    }
#if 0
    using enum e_ground_terrain;
    enum_map<e_ground_terrain, double> const targets{
      { e_ground_terrain::savannah, 1.083 },  //
      { e_ground_terrain::grassland, 1.131 }, //
      { e_ground_terrain::tundra, 1.754 },    //
      { e_ground_terrain::plains, 1.143 },    //
      { e_ground_terrain::prairie, 1.121 },   //
      { e_ground_terrain::desert, 1.210 },    //
      { e_ground_terrain::swamp, 1.067 },     //
      { e_ground_terrain::marsh, 1.250 },     //
      { e_ground_terrain::arctic, 1.000 },    //
    };
    fmt::println( "-----" );
    for( e_ground_terrain const gt : kGroundTypes ) {
      double const target = targets[gt];
      if( gt == e_ground_terrain::arctic ) continue;
      auto const g = relative_adjacencies[gt];
      fmt::println( "{}: target={}, actual={:.3}, error={:.3}%",
                    gt, target, g,
                    100.0 * ( g - target ) / target );
    }
#endif
  }

 private:
  size map_sz_ = {};
  enum_map<e_ground_terrain, int> adjacency_;
  enum_map<e_ground_terrain, int> terrain_total_;
  enum_map<e_ground_terrain, map<int /*y*/, int /*count*/>>
      ground_per_row_;
  map<int /*y*/, int /*count*/> land_per_row_;
  // This gives the total number of surrounding land squares (of
  // any biome) around tiles of the given biome on the given row.
  enum_map<e_ground_terrain, map<int /*y*/, int /*count*/>>
      surrounding_per_row_;
  int land_total_ = 0;
  int maps_total_ = 0;
};

/****************************************************************
** Runners.
*****************************************************************/
[[maybe_unused]] void testing_map_gen_biome_density_stats(
    IEngine& engine ) {
  int constexpr kNumSamples = 2000;

  auto const generate =
      [&]( SS& ss, ClassicGameSetupParamsCustom const& custom ) {
        generate_single_map_custom( engine, ss, custom );
      };

  static auto constexpr kTemps = {
    e_temperature::cool,
    e_temperature::temperate,
    e_temperature::warm,
  };
  static auto constexpr kClimates = {
    e_climate::arid,
    e_climate::normal,
    e_climate::wet,
  };

  for( e_temperature const temperature : kTemps ) {
    for( e_climate const climate : kClimates ) {
      string const name( mode_name( temperature, climate ) );
      BiomeDensityStats stats( name );
      fmt::println( "generate for {}...", name );
      ScopedTimer const timer(
          format( "generate {} maps", kNumSamples ) );
      for( int i = 0; i < kNumSamples; ++i ) {
        fmt::print( "generating map {}...", i + 1 );
        SS ss;
        generate( ss, { .land_mass   = e_land_mass::large,
                        .land_form   = e_land_form::continents,
                        .temperature = temperature,
                        .climate     = climate } );
        stats.collect( ss );
        // fmt::print( "\r\033[2K" );
        fmt::print( "\n" );
      }
      stats.write();
      fmt::print( "\n" );
    }
  }
}

[[maybe_unused]] void testing_map_gen_biome_adjacency_stats(
    IEngine& engine ) {
  int constexpr kNumSamples = 2000;

  auto const generate =
      [&]( SS& ss, ClassicGameSetupParamsCustom const& custom ) {
        generate_single_map_custom( engine, ss, custom );
      };

  static auto constexpr kTemps = {
    // e_temperature::cool,
    e_temperature::temperate,
    // e_temperature::warm,
  };
  static auto constexpr kClimates = {
    // e_climate::arid,
    e_climate::normal,
    // e_climate::wet,
  };

  for( e_temperature const temperature : kTemps ) {
    for( e_climate const climate : kClimates ) {
      string const name( mode_name( temperature, climate ) );
      BiomeAdjacencyStats stats;
      fmt::println( "generate for {}...", name );
      for( int i = 0; i < kNumSamples; ++i ) {
        // fmt::print( "generating map {}...", i + 1 );
        SS ss;
        generate( ss, { .land_mass   = e_land_mass::large,
                        .land_form   = e_land_form::continents,
                        .temperature = temperature,
                        .climate     = climate } );
        stats.collect( ss );
        // fmt::print( "\r\033[2K" );
        // fmt::print( "\n" );
      }
      stats.write();
      fmt::print( "\n" );
    }
  }
}

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
  CHECK_HAS_VALUE( validate_game_setup( setup ) );
}

void testing_map_gen( IEngine& engine, bool const reseed ) {
  SS ss;
  generate_single_map_key( engine, ss, reseed );
  print_ascii_map( ss.terrain.real_terrain(), cout );
}

void testing_map_gen_stats( IEngine& engine ) {
  // testing_map_gen_biome_density_stats( engine );

  testing_map_gen_biome_adjacency_stats( engine );
}

void drop_large_og_map( IEngine& engine ) {
  SS ss;
  generate_single_map_key( engine, ss,
                           /*reseed=*/true );
  size const sz = ss.terrain.world_size_tiles();
  sav::MapFile map_file;
  CHECK_HAS_VALUE( bridge::convert_map_to_og(
      ss.terrain.real_terrain(), map_file ) );
  string const filename =
      "/home/dsicilia/dev/revolution-now/LARGE.MP";
  CHECK_HAS_VALUE( sav::save_map_file( filename, map_file ) );
  print_ascii_map( ss.terrain.real_terrain(), cout );
  fmt::println( "saved OG map file of size {} to {}.", sz,
                filename );
}

} // namespace rn
