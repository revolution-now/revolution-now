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
#include "biomes.hpp"
#include "connectivity.hpp"
#include "create-game.hpp"
#include "game-setup.hpp"
#include "gnuplot.hpp"
#include "iengine.hpp"
#include "irand.hpp"
#include "lua.hpp"
#include "map-stats.hpp"
#include "map-updater.hpp"
#include "perlin-map.hpp"
#include "terrain-enums.rds.hpp"
#include "terrain-mgr.hpp"

// config
#include "config/map-gen.rds.hpp"

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
#include "base/scope-exit.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <fstream>
#include <iostream>
#include <thread>

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

auto const& ascii_map_formatter = ascii_map_rivers_formatter;

/****************************************************************
** Helpers.
*****************************************************************/
string mode_name( ClassicGameSetupParamsCustom const params ) {
  string res;
  res.resize( 4 );
  switch( params.climate ) {
    case e_climate::arid:
      res[3] = 't';
      break;
    case e_climate::normal:
      res[3] = 'm';
      break;
    case e_climate::wet:
      res[3] = 'b';
      break;
  }
  switch( params.temperature ) {
    case e_temperature::cool:
      res[2] = 't';
      break;
    case e_temperature::temperate:
      res[2] = 'm';
      break;
    case e_temperature::warm:
      res[2] = 'b';
      break;
  }
  switch( params.land_form ) {
    case e_land_form::archipelago:
      res[1] = 't';
      break;
    case e_land_form::normal:
      res[1] = 'm';
      break;
    case e_land_form::continents:
      res[1] = 'b';
      break;
  }
  switch( params.land_mass ) {
    case e_land_mass::small:
      res[0] = 't';
      break;
    case e_land_mass::moderate:
      res[0] = 'm';
      break;
    case e_land_mass::large:
      res[0] = 'b';
      break;
  }
  return res;
}

string_view mode_name( e_land_mass const m,
                       e_land_form const f ) {
  switch( m ) {
    case e_land_mass::small:
      switch( f ) {
        case e_land_form::archipelago:
          return "ttmm";
        case e_land_form::normal:
          return "tmmm";
        case e_land_form::continents:
          return "tbmm";
      }
      break;
    case e_land_mass::moderate:
      switch( f ) {
        case e_land_form::archipelago:
          return "mtmm";
        case e_land_form::normal:
          return "mmmm";
        case e_land_form::continents:
          return "mbmm";
      }
      break;
    case e_land_mass::large:
      switch( f ) {
        case e_land_form::archipelago:
          return "btmm";
        case e_land_form::normal:
          return "bmmm";
        case e_land_form::continents:
          return "bbmm";
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
  CHECK_HAS_VALUE( validate_game_setup( setup ) );

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
    if( reseed )
      // Reseed all seeds in the structure, but keep all other
      // parameters the same.
      randomize_game_setup_seeds( rand, setup );
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
struct LandDensityStats : IMapStatsCollector {
  void collect( MapMatrix const& m ) override {
    map_sz_ = m.size();
    ++maps_total_;
    on_all_tiles(
        m, [&]( point const tile, MapSquare const& square ) {
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

  void summarize() override {}

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

struct LakeFrequencyStats : IMapStatsCollector {
  void collect( MapMatrix const& m ) override {
    CHECK( map_sz_ == size{} or map_sz_ == m.size() );
    map_sz_ = m.size();
    ++total_maps_;
    TerrainConnectivity const connectivity =
        compute_terrain_connectivity( m );
    on_all_tiles(
        m, [&]( point const tile, MapSquare const& center ) {
          // Skip arctic rows.
          if( tile.y == 0 || tile.y == map_sz_.h - 1 ) return;
          if( center.surface == e_surface::land ) {
            // Land.
            ++total_land_;
            bool has_adjacent_water = false;
            on_surrounding(
                m, tile,
                [&]( point const, MapSquare const& adjacent ) {
                  if( adjacent.surface == e_surface::water )
                    has_adjacent_water = true;
                } );
            if( has_adjacent_water )
              ++total_land_tiles_adjacent_to_water_;
          } else {
            // Water.
            ++total_water_;
            bool has_adjacent_land = false;
            on_surrounding(
                m, tile,
                [&]( point const, MapSquare const& adjacent ) {
                  if( adjacent.surface == e_surface::land )
                    has_adjacent_land = true;
                } );
            if( has_adjacent_land )
              ++total_water_tiles_adjacent_to_land_;
            bool const is_inland =
                is_inland_lake( connectivity, tile );
            if( is_inland ) ++total_inland_water_tiles_;
          }
        } );
  }

  void summarize() override {}

  void write() const override {
    double const metric = [&] {
      double const land_density =
          double( total_land_ ) / ( total_land_ + total_water_ );
      return pow( 1.0 / land_density, 1.5 ) *
             total_inland_water_tiles_ / total_land_;
    }();
    double const total_inland_water_tiles_per_map =
        double( total_inland_water_tiles_ ) / total_maps_;

    fmt::println( "total_maps:                       {}",
                  total_maps_ );
    fmt::println( "total_inland_water_tiles_per_map: {:.1f}",
                  total_inland_water_tiles_per_map );
    fmt::println( "metric:                           {:.3f}",
                  metric );
  }

 private:
  size map_sz_                            = {};
  int total_maps_                         = 0;
  int total_land_                         = 0;
  int total_water_                        = 0;
  int total_inland_water_tiles_           = 0;
  int total_water_tiles_adjacent_to_land_ = 0;
  int total_land_tiles_adjacent_to_water_ = 0;
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
      ClassicGameSetupParamsCustom const params{
        .land_mass   = e_land_mass::large,
        .land_form   = e_land_form::continents,
        .temperature = temperature,
        .climate     = climate };
      string const name( mode_name( params ) );
      auto const stats =
          create_biome_density_stats_collector( name );
      CHECK( stats );
      fmt::println( "generate for {}...", name );
      ScopedTimer const timer(
          format( "generate {} maps", kNumSamples ) );
      for( int i = 0; i < kNumSamples; ++i ) {
        fmt::print( "generating map {}...", i + 1 );
        SS ss;
        generate( ss, params );
        stats->collect( ss.terrain.real_terrain().map );
        // fmt::print( "\r\033[2K" );
        fmt::print( "\n" );
      }
      stats->summarize();
      stats->write();
      fmt::print( "\n" );
    }
  }
}

[[maybe_unused]] void testing_map_gen_wetness_stats(
    IEngine& engine ) {
  int constexpr kNumSamples = 2000;

  auto const generate =
      [&]( SS& ss, ClassicGameSetupParamsCustom const& custom ) {
        generate_single_map_custom( engine, ss, custom );
      };

  using M = e_land_mass;
  using F = e_land_form;
  using T = e_temperature;
  using C = e_climate;

  static auto constexpr kModes = {
    // clang-format off
      tuple{ M::small,    F::archipelago, T::cool,      C::arid   },
      tuple{ M::small,    F::archipelago, T::temperate, C::arid   },
      tuple{ M::small,    F::archipelago, T::temperate, C::normal },
      tuple{ M::small,    F::archipelago, T::temperate, C::wet    },
      tuple{ M::small,    F::archipelago, T::warm,      C::wet    },

      tuple{ M::moderate, F::normal,      T::cool,      C::arid   },
      tuple{ M::moderate, F::normal,      T::temperate, C::arid   },
      tuple{ M::moderate, F::normal,      T::temperate, C::normal },
      tuple{ M::moderate, F::normal,      T::temperate, C::wet    },
      tuple{ M::moderate, F::normal,      T::warm,      C::wet    },

      tuple{ M::large,    F::continents,  T::cool,      C::arid   },
      tuple{ M::large,    F::continents,  T::temperate, C::arid   },
      tuple{ M::large,    F::continents,  T::temperate, C::normal },
      tuple{ M::large,    F::continents,  T::temperate, C::wet    },
      tuple{ M::large,    F::continents,  T::warm,      C::wet    },
    // clang-format on
  };

  for( auto const& [land_mass, land_form, temperature, climate] :
       kModes ) {
    ClassicGameSetupParamsCustom const params{
      .land_mass   = land_mass,
      .land_form   = land_form,
      .temperature = temperature,
      .climate     = climate };
    string const name( mode_name( params ) );
    auto const stats = create_wetness_stats_collector(
        name, config_map_gen.terrain_generation.weather.climate
                  .customized[climate] );
    fmt::println( "generating for {}...", name );
    fmt::print( "\033[?25l" );
    SCOPE_EXIT { fmt::print( "\033[?25h" ); };
    for( int i = 0; i < kNumSamples; ++i ) {
      fmt::print( "  #{} ({:.3}%)          \r", i,
                  i * 100.0 / kNumSamples );
      SS ss;
      generate( ss, params );
      stats->collect( ss.terrain.real_terrain().map );
    }
    stats->summarize();
    stats->write();
    fmt::print( "\n" );
  }
}

[[maybe_unused]] void testing_map_gen_lake_stats(
    IEngine& engine ) {
  int constexpr kNumSamples = 2000;

  auto const generate =
      [&]( SS& ss, ClassicGameSetupParamsCustom const& custom ) {
        generate_single_map_custom( engine, ss, custom );
      };

  static auto constexpr kModes = {
    pair{ e_land_mass::small, e_land_form::archipelago },
    pair{ e_land_mass::moderate, e_land_form::normal },
    pair{ e_land_mass::large, e_land_form::continents },
    pair{ e_land_mass::moderate, e_land_form::archipelago },
    pair{ e_land_mass::moderate, e_land_form::continents },
    pair{ e_land_mass::small, e_land_form::normal },
    pair{ e_land_mass::large, e_land_form::normal },
    pair{ e_land_mass::small, e_land_form::continents },
    pair{ e_land_mass::large, e_land_form::archipelago },
  };

  for( auto const& [land_mass, land_form] : kModes ) {
    string const name( mode_name( land_mass, land_form ) );
    LakeFrequencyStats stats;
    fmt::println( "generating for {}...", name );
    for( int i = 0; i < kNumSamples; ++i ) {
      SS ss;
      generate( ss, { .land_mass   = land_mass,
                      .land_form   = land_form,
                      .temperature = e_temperature::temperate,
                      .climate     = e_climate::normal } );
      stats.collect( ss.terrain.real_terrain().map );
    }
    stats.summarize();
    stats.write();
    fmt::print( "\n" );
  }
}

[[maybe_unused]] void testing_map_gen_river_stats(
    IEngine& engine ) {
  int constexpr kNumSamples = 10000;

  auto const generate =
      [&]( SS& ss, ClassicGameSetupParamsCustom const& custom ) {
        generate_single_map_custom( engine, ss, custom );
      };

  auto const generate_new = [&]( SS& ss ) {
    generate_single_map_new( engine, ss );
  };

  bool const kDoCustom = true;
  bool const kDoNew    = true;

  vector<thread> ths;

  if( kDoCustom ) {
    using enum e_climate;
    using enum e_land_mass;
    static auto constexpr kModes = {
      tuple{ moderate, e_land_form::archipelago, arid, .15 },
      tuple{ moderate, e_land_form::archipelago, normal, .15 },
      tuple{ moderate, e_land_form::archipelago, wet, .15 },
      tuple{ moderate, e_land_form::normal, arid, .05 },
      tuple{ moderate, e_land_form::normal, normal, .05 },
      tuple{ moderate, e_land_form::normal, wet, .05 },
      tuple{ moderate, e_land_form::continents, arid, .15 },
      tuple{ moderate, e_land_form::continents, normal, .15 },
      tuple{ moderate, e_land_form::continents, wet, .15 },
      tuple{ large, e_land_form::continents, normal, .15 },
    };

    for( auto const& tpl : kModes ) {
      ths.emplace_back( [tpl, &generate] {
        auto const [land_mass, land_form, climate, tolerance] =
            tpl;
        ClassicGameSetupParamsCustom const params{
          .land_mass   = land_mass,
          .land_form   = land_form,
          .temperature = e_temperature::temperate,
          .climate     = climate };
        string const name = mode_name( params );
        auto const stats =
            create_river_stats_collector( name, tolerance );
        fmt::println( "generating for {}...", name );
        for( int i = 0; i < kNumSamples; ++i ) {
          SS ss;
          generate( ss, params );
          stats->collect( ss.terrain.real_terrain().map );
        }
        stats->summarize();
        stats->write();
      } );
    }
  }

  if( kDoNew ) {
    ths.emplace_back( [&generate_new] {
      string const name = "new";
      auto const stats =
          create_river_stats_collector( name, .05 );
      fmt::println( "generating for {}...", name );
      for( int i = 0; i < kNumSamples; ++i ) {
        SS ss;
        generate_new( ss );
        stats->collect( ss.terrain.real_terrain().map );
      }
      stats->summarize();
      stats->write();
    } );
  }

  for( thread& th : ths ) th.join();
}

[[maybe_unused]] void testing_map_gen_formation_stats(
    IEngine& engine ) {
  int constexpr kNumSamples = 10000;

  auto const generate =
      [&]( SS& ss, ClassicGameSetupParamsCustom const& custom ) {
        generate_single_map_custom( engine, ss, custom );
      };

  using M = e_land_mass;
  using F = e_land_form;
  using T = e_temperature;
  using C = e_climate;

  static auto constexpr kModes = {
    // clang-format off
        tuple{ M::small,    F::archipelago, T::temperate, C::normal },
        tuple{ M::moderate, F::normal,      T::temperate, C::normal },
        tuple{ M::large,    F::continents,  T::temperate, C::normal },
        tuple{ M::moderate, F::archipelago, T::temperate, C::normal },
        tuple{ M::moderate, F::continents,  T::temperate, C::normal },
        tuple{ M::small,    F::normal,      T::temperate, C::normal },
        tuple{ M::large,    F::normal,      T::temperate, C::normal },
        tuple{ M::small,    F::continents,  T::temperate, C::normal },
        tuple{ M::large,    F::archipelago, T::temperate, C::normal },
        tuple{ M::large,    F::continents,  T::cool,      C::arid   },
        tuple{ M::large,    F::continents,  T::temperate, C::arid   },
        tuple{ M::large,    F::continents,  T::warm,      C::arid   },
        tuple{ M::large,    F::continents,  T::cool,      C::normal },
        tuple{ M::large,    F::continents,  T::temperate, C::normal },
        tuple{ M::large,    F::continents,  T::warm,      C::normal },
        tuple{ M::large,    F::continents,  T::cool,      C::wet    },
        tuple{ M::large,    F::continents,  T::temperate, C::wet    },
        tuple{ M::large,    F::continents,  T::warm,      C::wet    },
    // clang-format on
  };

  for( auto const& [land_mass, land_form, temperature, climate] :
       kModes ) {
    ClassicGameSetupParamsCustom const params{
      .land_mass   = land_mass,
      .land_form   = land_form,
      .temperature = temperature,
      .climate     = climate };
    string const name( mode_name( params ) );
    auto const stats = create_formations_stats_collector( name );
    CHECK( stats );
    fmt::println( "generate for {}...", name );
    ScopedTimer const timer(
        format( "generate {} maps", kNumSamples ) );
    fmt::print( "\033[?25l" );
    SCOPE_EXIT { fmt::print( "\033[?25h" ); };
    for( int i = 0; i < kNumSamples; ++i ) {
      fmt::print( "  #{} ({:.3}%)          \r", i + 1,
                  i * 100.0 / kNumSamples );
      SS ss;
      generate( ss, params );
      stats->collect( ss.terrain.real_terrain().map );
    }
    stats->summarize();
    stats->write();
    fmt::print( "\n" );
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

void testing_map_gen_key( IEngine& engine, bool const reseed ) {
  SS ss;
  generate_single_map_key( engine, ss, reseed );
  print_ascii_map( ss.terrain.real_terrain(),
                   ascii_map_formatter(), cout );
}

void testing_map_gen_fuzz( IEngine&, bool const ) {
}

void testing_map_gen_custom( IEngine& engine ) {
  SS ss;
  ClassicGameSetupParamsCustom const params{
    .land_mass   = e_land_mass::large,
    .land_form   = e_land_form::continents,
    .temperature = e_temperature::temperate,
    .climate     = e_climate::normal };
  fmt::println( "mode: {}", mode_name( params ) );
  generate_single_map_custom( engine, ss, params );
  print_ascii_map( ss.terrain.real_terrain(),
                   ascii_map_formatter(), cout );
}

void testing_map_gen_default( IEngine& engine ) {
  SS ss;
  fmt::println( "mode: new" );
  generate_single_map_new( engine, ss );
  print_ascii_map( ss.terrain.real_terrain(),
                   ascii_map_formatter(), cout );
}

void testing_map_gen_stats( IEngine& engine ) {
  base::e_log_level const old_level = base::global_log_level();
  set_global_log_level( base::e_log_level::warn );
  SCOPE_EXIT { set_global_log_level( old_level ); };
  // testing_map_gen_biome_density_stats( engine );
  // testing_map_gen_wetness_stats( engine );
  // testing_map_gen_lake_stats( engine );
  testing_map_gen_river_stats( engine );
  // testing_map_gen_formation_stats( engine );
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
  print_ascii_map( ss.terrain.real_terrain(),
                   ascii_map_formatter(), cout );
  fmt::println( "saved OG map file of size {} to {}.", sz,
                filename );
}

} // namespace rn
