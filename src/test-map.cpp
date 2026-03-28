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
#include "rcl/emit.hpp"
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
    UNWRAP_CHECK_T(
        auto& generate_native,
        setup.map.source.get_if<MapSource::generate_native>() );
    auto& native = generate_native.setup;
    if( reseed ) {
      auto const S = [&] {
        return rand.new_deterministic_seed();
      };
      auto& surf_gen = native.surface_generator;

      surf_gen.perlin_settings.seed =
          generate_perlin_seed( S() );
      surf_gen.lakes.seed  = S();
      native.rivers.seed   = S();
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

struct RiverFrequencyStats : IMapStatsCollector {
  RiverFrequencyStats( string const& mode,
                       double const tolerance )
    : mode_( mode ), tolerance_( tolerance ) {
    CHECK( !mode_.empty() );
  }

  void collect( MapMatrix const& m ) override {
    __river_tile_segments_ = {};
    __segments             = {};
    __any_                 = {};
    __num_connected_       = {};

    ++maps_;
    CHECK( map_sz_ == size{} or map_sz_ == m.size() );
    map_sz_ = m.size();

    auto fn = [&]( point const tile, MapSquare const& center ) {
      using enum e_surface;
      using enum e_river;

      // Skip arctic rows.
      if( tile.y == 0 || tile.y == map_sz_.h - 1 ) return;

      ++tiles_;

      maj_by_row_[tile.y];
      min_by_row_[tile.y];
      any_by_row_[tile.y];
      any_on_land_by_row_[tile.y];
      land_by_row_[tile.y];
      water_adjacent_to_land_by_row_[tile.y];
      starts_by_row_[tile.y];

      if( center.surface == water ) {
        ++water_;
        bool has_adjacent_land = false;
        on_surrounding_cardinal(
            m, tile,
            [&]( point const p, MapSquare const& adjacent,
                 e_cardinal_direction const ) {
              // Skip arctic rows.
              if( p.y == 0 || p.y == map_sz_.h - 1 ) return;
              if( adjacent.surface == land )
                has_adjacent_land = true;
            } );
        if( has_adjacent_land ) {
          ++water_adjacent_to_land_;
          ++water_adjacent_to_land_by_row_[tile.y];
        }
      }

      if( center.surface == land ) {
        ++land_;
        ++land_by_row_[tile.y];
      }

      if( center.river.has_value() ) {
        __river_tile_segments_[tile] = 0;
        ++any_;
        ++__any_;
        ++any_by_row_[tile.y];
        if( center.river == minor ) {
          ++min_;
          ++min_by_row_[tile.y];
        }
        if( center.river == major ) {
          ++maj_;
          ++maj_by_row_[tile.y];
        }
        int rivers_surrounding = 0;
        on_surrounding_cardinal(
            m, tile,
            [&]( point const p, MapSquare const& adjacent,
                 e_cardinal_direction const ) {
              // Skip arctic rows.
              if( p.y == 0 || p.y == map_sz_.h - 1 ) return;
              if( adjacent.river.has_value() )
                ++rivers_surrounding;
            } );
        if( rivers_surrounding == 3 ) {
          ++any_forks_;
          if( center.river == minor ) ++min_forks_;
          if( center.river == major ) ++maj_forks_;
        }
        if( rivers_surrounding == 4 ) {
          ++any_crosses_;
          if( center.river == minor ) ++min_crosses_;
          if( center.river == major ) ++maj_crosses_;
        }
        if( center.surface == water ) {
          ++any_on_water_;
          ++starts_;
          ++starts_by_row_[tile.y];
          if( center.river == minor ) ++min_on_water_;
          if( center.river == major ) ++maj_on_water_;
          if( rivers_surrounding == 0 ) {
            ++water_islands_;
            ++any_islands_;
          }
        }
        if( center.surface == land ) {
          ++any_on_land_by_row_[tile.y];
          if( rivers_surrounding == 0 ) {
            ++land_islands_;
            ++any_islands_;
          }
          if( rivers_surrounding == 1 ) ++ends_;
          if( rivers_surrounding == 2 ) {
            auto const has_river = [&]( point const p ) {
              return m[p].river.has_value();
            };
            bool const has_top_bottom =
                has_river( tile.moved_up() ) &&
                has_river( tile.moved_down() );
            bool const has_left_right =
                has_river( tile.moved_left() ) &&
                has_river( tile.moved_right() );
            bool const is_turn =
                not has_top_bottom and not has_left_right;
            if( is_turn ) ++turns_;
          }
        }
      }
    };
    on_all_tiles( m, fn );

    find_connected( m );

    any_squared_           = any_squared_ + __any_ * __any_;
    num_connected_squared_ = num_connected_squared_ +
                             __num_connected_ * __num_connected_;
  }

  void summarize() override {}

  static double compute_stddev( double const s1, double const s2,
                                double const denominator ) {
    return pow( s2 / denominator - pow( s1 / denominator, 2 ),
                .5 );
  }

  void assign_segment( MapMatrix const& m, point const start,
                       int const segment ) {
    using enum e_surface;
    using enum e_river;
    // Should not modify __river_tile_segments_ in this function
    // except when setting the start tile. since we are iterating
    // over it in the calling function.
    auto const& rts        = __river_tile_segments_;
    auto const look_up_rts = [&]( point const p ) -> maybe<int> {
      return base::lookup( rts, p );
    };
    auto const set_rts = [&]( point const p, int const val ) {
      CHECK( p == start );
      __river_tile_segments_[p] = val;
    };
    CHECK( look_up_rts( start ) == 0,
           "segment for tile {} is not zero, instead it is {}.",
           start, look_up_rts( start ) );
    CHECK( segment > 0 );
    set_rts( start, segment );
    CHECK( m[start].river.has_value() );
    bool const is_this_water = m[start].surface == water;
    auto fn = [&]( point const p, MapSquare const& adjacent,
                   e_cardinal_direction const ) {
      bool const is_surrounding_water =
          adjacent.surface == water;
      // Can't connect a single river segment across two water
      // tiles.
      if( is_this_water && is_surrounding_water ) return;
      if( auto const v = look_up_rts( p );
          v.has_value() && *v > 0 ) {
        CHECK( adjacent.river.has_value() );
        CHECK( *v == segment );
        return;
      }
      if( adjacent.river.has_value() )
        assign_segment( m, p, segment );
    };
    on_surrounding_cardinal( m, start, fn );
  }

  void find_connected( MapMatrix const& m ) {
    for( auto const& [tile, segment] : __river_tile_segments_ ) {
      MapSquare const& square = m[tile];
      CHECK( square.river.has_value(), "tile {} has no river.",
             tile );
      if( segment == 0 ) {
        ++__segments;
        assign_segment( m, tile, __segments );
      }
    }
    map<int /*segment*/, int> segment_length;
    map<int /*segment*/, bool> segment_has_water;
    auto fn = [&]( point const tile, MapSquare const& center ) {
      using enum e_surface;
      using enum e_river;
      if( !center.river.has_value() ) return;
      int const segment = __river_tile_segments_[tile];
      CHECK( segment > 0 );
      ++segment_length[segment];
      if( center.surface == water )
        segment_has_water[segment] = true;
    };
    on_all_tiles( m, fn );

    int max_length = 0;
    for( int segment = 1; segment <= __segments; ++segment ) {
      int const length = segment_length[segment];
      max_length       = std::max( max_length, length );
      if( length > ssize( lengths_ ) - 1 )
        lengths_.resize( length + 1 );
      CHECK_LT( length, ssize( lengths_ ) );
      ++lengths_[length];
      if( segment_has_water[segment] )
        ++num_with_water_;
      else
        ++num_without_water_;
    }
    num_connected_ += __segments;
    __num_connected_ = __segments;
  }

  void write() const override {
    double const maps  = maps_;
    double const tiles = tiles_ / maps;

    double const land  = land_ / maps;
    double const water = water_ / maps;
    double const water_adjacent_to_land =
        water_adjacent_to_land_ / maps;

    double const maj = maj_ / maps;
    double const min = min_ / maps;
    double const any = any_ / maps;
    double const any_stddev =
        compute_stddev( any_, any_squared_, maps );
    double const any_per_land = any_ / double( land_ );

    double const maj_on_water = maj_on_water_ / maps;
    double const min_on_water = min_on_water_ / maps;
    double const any_on_water = any_on_water_ / maps;

    double const maj_forks = maj_forks_ / maps;
    double const min_forks = min_forks_ / maps;
    double const any_forks = any_forks_ / maps;

    double const maj_crosses = maj_crosses_ / maps;
    double const min_crosses = min_crosses_ / maps;
    double const any_crosses = any_crosses_ / maps;

    double const any_forks_per_connected =
        num_connected_ > 0
            ? any_forks_ / double( num_connected_ )
            : 0;

    double const any_crosses_per_connected =
        num_connected_ > 0
            ? any_crosses_ / double( num_connected_ )
            : 0;

    double const num_connected        = num_connected_ / maps;
    double const num_connected_stddev = compute_stddev(
        num_connected_, num_connected_squared_, maps );
    double const num_connected_per_shore =
        num_connected_ / double( water_adjacent_to_land_ );
    double const num_connected_per_land =
        num_connected_ / double( land_ );

    double const num_with_water    = num_with_water_ / maps;
    double const num_without_water = num_without_water_ / maps;

    double const starts = starts_ / maps;
    double const ends   = ends_ / maps;
    double const turns  = turns_ / maps;

    double const turns_per_connected =
        num_connected_ > 0 ? turns_ / double( num_connected_ )
                           : 0;

    double const land_islands  = land_islands_ / maps;
    double const water_islands = water_islands_ / maps;
    double const any_islands   = any_islands_ / maps;

    double length_avg = 0;
    int total_count   = 0;
    for( int length = 1; length < ssize( lengths_ ); ++length ) {
      int const count = lengths_[length];
      total_count += count;
      length_avg += length * count;
    }
    length_avg /= total_count;

    cdr::table o;
    o["savs"]                      = int( maps );
    o["length_avg"]                = length_avg;
    o["any"]                       = any;
    o["any_stddev"]                = any_stddev;
    o["num_connected"]             = num_connected;
    o["num_connected_stddev"]      = num_connected_stddev;
    o["num_connected_per_shore"]   = num_connected_per_shore;
    o["num_connected_per_land"]    = num_connected_per_land;
    o["land"]                      = land;
    o["water_adjacent_to_land"]    = water_adjacent_to_land;
    o["maj"]                       = maj;
    o["min"]                       = min;
    o["any_per_land"]              = any_per_land;
    o["maj_on_water"]              = maj_on_water;
    o["min_on_water"]              = min_on_water;
    o["any_on_water"]              = any_on_water;
    o["turns"]                     = turns;
    o["turns_per_connected"]       = turns_per_connected;
    o["tiles"]                     = tiles;
    o["water"]                     = water;
    o["maj_forks"]                 = maj_forks;
    o["min_forks"]                 = min_forks;
    o["any_forks"]                 = any_forks;
    o["any_forks_per_connected"]   = any_forks_per_connected;
    o["maj_crosses"]               = maj_crosses;
    o["min_crosses"]               = min_crosses;
    o["any_crosses"]               = any_crosses;
    o["any_crosses_per_connected"] = any_crosses_per_connected;
    o["num_with_water"]            = num_with_water;
    o["num_without_water"]         = num_without_water;
    o["starts"]                    = starts;
    o["ends"]                      = ends;
    o["land_islands"]              = land_islands;
    o["water_islands"]             = water_islands;
    o["any_islands"]               = any_islands;
    string const out_filename      = format(
        "tools/auto-measure/auto-map-gen/rivers/generated/"
             "{}.data.json",
        mode_ );
    ofstream out( out_filename );
    CHECK( out.good() );
    out << rcl::emit_json( o );

    string const in_filename = format(
        "tools/auto-measure/auto-map-gen/rivers/empirical/"
        "{}.data.json",
        mode_ );
    UNWRAP_CHECK_T( rcl::doc const& empirical_doc,
                    rcl::parse_file( in_filename ) );
    cdr::table const& empirical = empirical_doc.top_tbl();

    {
      cdr::list const kDiffColumns = {
        "length_avg",
        "num_connected_per_shore",
        "num_connected_per_land",
        "any_per_land",
        "turns_per_connected",
        "any_forks_per_connected",
        "num_connected",
        "turns",
        "maj",
        "any",
        "maj_on_water",
        "any_on_water",
        "maj_forks",
        "any_forks",
        "land",
      };

      cdr::table diff;
      cdr::table& fail = diff["fail"].emplace<cdr::table>();
      cdr::table& good = diff["good"].emplace<cdr::table>();
      for( cdr::value const& col : kDiffColumns ) {
        string const& colstr = col.get<string>();
        UNWRAP_CONTINUE( double const l,
                         o[colstr].get_if<double>() );
        UNWRAP_CONTINUE( double const r,
                         empirical[colstr].get_if<double>() );
        double const ratio = l / r;
        if( abs( ratio - 1.0 ) < tolerance_ )
          good[colstr] = ratio;
        else
          fail[colstr] = ratio;
      }
      fail["__key_order"]            = kDiffColumns;
      good["__key_order"]            = kDiffColumns;
      string const diff_out_filename = format(
          "tools/auto-measure/auto-map-gen/rivers/generated/"
          "{}.diff.json",
          mode_ );
      ofstream diff_out( diff_out_filename );
      CHECK( diff_out.good() );
      diff_out << rcl::emit_json(
          diff, rcl::JsonEmitOptions{ .key_order_tag =
                                          "__key_order" } );
    }

    write_per_row_plot();

    write_length_plot();
  }

  fs::path const kGeneratedDir =
      "tools/auto-measure/auto-map-gen/rivers/generated";

  void write_per_row_plot() const {
    GnuPlotSettings const settings{
      .title   = format( "River Density (generated) ({}) [{}]",
                         mode_, maps_ ),
      .x_label = "Y (map row)",
      .y_label = "Count Per Map",
      .x_range = "1:70",
      .y_range = "0:10",
    };

    CsvData csv_data{
      .header =
          {
            "y",                      //
            "any-by-row",             //
            "min-by-row",             //
            "maj-by-row",             //
            "starts-by-row",          //
            "starts-by-row-adjusted", //
            "any-by-row-adjusted",    //
          },
      .rows = {},
    };

    auto const row_getter = []( int const y,
                                auto const& m ) -> double {
      auto const it = m.find( y );
      CHECK( it != m.end(), "failed to find row {} in map", y );
      return it->second;
    };

    for( int y = 1; y < map_sz_.h - 1; ++y ) {
      auto const Y = bind_front( row_getter, y );
      vector<string> row{
        /*y=*/to_string( y + 1 ),
        /*any-by-row=*/
        to_string( Y( any_by_row_ ) / double( maps_ ) ),
        /*min-by-row=*/
        to_string( Y( min_by_row_ ) / double( maps_ ) ),
        /*maj-by-row=*/
        to_string( Y( maj_by_row_ ) / double( maps_ ) ),
        /*starts-by-row=*/
        to_string( Y( starts_by_row_ ) / double( maps_ ) ),
        /*starts-by-row-adjusted=*/
        to_string( 100 * Y( starts_by_row_ ) /
                   Y( water_adjacent_to_land_by_row_ ) ),
        /*any-by-row-adjusted=*/
        to_string( 50 * Y( any_on_land_by_row_ ) /
                   Y( land_by_row_ ) ),
      };
      csv_data.rows.push_back( std::move( row ) );
    }

    generate_gnuplot( kGeneratedDir, format( "{}.rows", mode_ ),
                      settings, csv_data );
  }

  void write_length_plot() const {
    GnuPlotSettings const settings{
      .title =
          format( "River Length Histogram (generated) ({}) [{}]",
                  mode_, maps_ ),
      .x_label = "Length",
      .y_label = "Count Per Map",
      .x_range = "1:20",
      .y_range = "0:20",
    };

    CsvData csv_data{
      .header = { "length", "count" },
      .rows   = {},
    };

    for( int length = 1; length < ssize( lengths_ ); ++length ) {
      int const count            = lengths_[length];
      double const per_map_count = double( count ) / maps_;
      vector<string> row{ to_string( length ),
                          to_string( per_map_count ) };
      csv_data.rows.push_back( std::move( row ) );
    }

    generate_gnuplot( kGeneratedDir,
                      format( "{}.lengths", mode_ ), settings,
                      csv_data );
  }

 private:
  string const mode_;
  double const tolerance_;

  // These need to be reset on each map.
  int __segments = 0;
  map<point, int /*segment*/> __river_tile_segments_;

  size map_sz_ = {};

  int maps_  = 0;
  int tiles_ = 0;

  int land_                   = 0;
  int water_                  = 0;
  int water_adjacent_to_land_ = 0;
  map<int /*y*/, int /*count*/> water_adjacent_to_land_by_row_;
  map<int /*y*/, int /*count*/> land_by_row_;

  int maj_         = 0;
  int min_         = 0;
  int any_         = 0;
  int __any_       = 0;
  int any_squared_ = 0;

  int maj_on_water_ = 0;
  int min_on_water_ = 0;
  int any_on_water_ = 0;

  map<int /*y*/, int /*count*/> maj_by_row_;
  map<int /*y*/, int /*count*/> min_by_row_;
  map<int /*y*/, int /*count*/> any_by_row_;
  map<int /*y*/, int /*count*/> any_on_land_by_row_;

  int maj_forks_ = 0;
  int min_forks_ = 0;
  int any_forks_ = 0;

  int maj_crosses_ = 0;
  int min_crosses_ = 0;
  int any_crosses_ = 0;

  int num_connected_         = 0;
  int __num_connected_       = 0;
  int num_connected_squared_ = 0;
  int num_with_water_        = 0;
  int num_without_water_     = 0;

  vector<int> lengths_; // idx=length, value=count

  int starts_ = 0;
  int ends_   = 0;
  int turns_  = 0;
  map<int /*y*/, int /*count*/> starts_by_row_;

  int land_islands_  = 0;
  int water_islands_ = 0;
  int any_islands_   = 0;
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
      BiomeDensityStatsCollector stats( name );
      fmt::println( "generate for {}...", name );
      ScopedTimer const timer(
          format( "generate {} maps", kNumSamples ) );
      for( int i = 0; i < kNumSamples; ++i ) {
        fmt::print( "generating map {}...", i + 1 );
        SS ss;
        generate( ss, params );
        stats.collect( ss.terrain.real_terrain().map );
        // fmt::print( "\r\033[2K" );
        fmt::print( "\n" );
      }
      stats.summarize();
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
      ClassicGameSetupParamsCustom const params{
        .land_mass   = e_land_mass::large,
        .land_form   = e_land_form::continents,
        .temperature = temperature,
        .climate     = climate };
      string const name( mode_name( params ) );
      BiomeClustering const clustering =
          biome_clustering_for_climate( climate );
      BiomeAdjacencyStatsCollector stats( clustering );
      fmt::println( "generating for {}...", name );
      for( int i = 0; i < kNumSamples; ++i ) {
        SS ss;
        generate( ss, params );
        stats.collect( ss.terrain.real_terrain().map );
        // fmt::println( "#{}", i + 1 );
      }
      stats.summarize();
      stats.write();
      fmt::print( "\n" );
    }
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
        RiverFrequencyStats stats( name, tolerance );
        fmt::println( "generating for {}...", name );
        for( int i = 0; i < kNumSamples; ++i ) {
          SS ss;
          generate( ss, params );
          stats.collect( ss.terrain.real_terrain().map );
        }
        stats.summarize();
        stats.write();
      } );
    }
  }

  if( kDoNew ) {
    ths.emplace_back( [&generate_new] {
      string const name = "new";
      RiverFrequencyStats stats( name, .05 );
      fmt::println( "generating for {}...", name );
      for( int i = 0; i < kNumSamples; ++i ) {
        SS ss;
        generate_new( ss );
        stats.collect( ss.terrain.real_terrain().map );
      }
      stats.summarize();
      stats.write();
    } );
  }

  for( thread& th : ths ) th.join();
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
                   ascii_map_biome_formatter(), cout );
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
                   ascii_map_biome_formatter(), cout );
}

void testing_map_gen_default( IEngine& engine ) {
  SS ss;
  fmt::println( "mode: new" );
  generate_single_map_new( engine, ss );
  print_ascii_map( ss.terrain.real_terrain(),
                   ascii_map_biome_formatter(), cout );
}

void testing_map_gen_stats( IEngine& engine ) {
  base::e_log_level const old_level = base::global_log_level();
  set_global_log_level( base::e_log_level::warn );
  SCOPE_EXIT { set_global_log_level( old_level ); };
  // testing_map_gen_biome_density_stats( engine );
  testing_map_gen_biome_adjacency_stats( engine );
  // testing_map_gen_lake_stats( engine );
  // testing_map_gen_river_stats( engine );
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
                   ascii_map_biome_formatter(), cout );
  fmt::println( "saved OG map file of size {} to {}.", sz,
                filename );
}

} // namespace rn
