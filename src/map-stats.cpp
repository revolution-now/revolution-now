/****************************************************************
**map-stats.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-29.
*
* Description: Some map statistics collectors.
*
*****************************************************************/
#include "map-stats.hpp"

// Revolution Now
#include "gnuplot.hpp"
#include "map-gen.hpp"
#include "terrain-mgr.hpp"

// config
#include "config/map-gen-types.hpp"
#include "config/map-gen.rds.hpp"

// ss
#include "ss/map-matrix.hpp"
#include "ss/terrain-enums.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// refl
#include "refl/enum-map.hpp"

// rcl
#include "rcl/emit.hpp"

// cdr
#include "cdr/ext-builtin.hpp"

// base
#include "base/ansi.hpp"
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"

// C++ standard library
#include <fstream>

namespace rn {

namespace {

using namespace std;

using ::base::lookup;
using ::base::maybe;
using ::base::nothing;
using ::base::str_replace_all;
using ::gfx::matrix;
using ::gfx::point;
using ::gfx::size;
using ::refl::enum_count;
using ::refl::enum_from_string;
using ::refl::enum_map;
using ::refl::enum_values;

/****************************************************************
** Global constants.
*****************************************************************/
static array<e_biome, 9> constexpr kGroundTypes = {
  e_biome::savannah, e_biome::grassland, e_biome::tundra,
  e_biome::plains,   e_biome::prairie,   e_biome::desert,
  e_biome::swamp,    e_biome::marsh,     e_biome::arctic,
};
static_assert( kGroundTypes.size() == enum_count<e_biome> );

/****************************************************************
** BiomeDensityStatsCollector
*****************************************************************/
struct BiomeDensityStatsCollector : IMapStatsCollector {
  BiomeDensityStatsCollector( std::string const& stem );

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

 private:
  std::string const stem_;
  size map_sz_ = {};
  std::map<int /*y+1*/, std::map<e_biome, int /*count*/>>
      biome_count_y_;
  std::map<int /*y+1*/, int> land_count_y_;
  int land_total_ = 0;
  int maps_total_ = 0;
};

BiomeDensityStatsCollector::BiomeDensityStatsCollector(
    string const& stem )
  : stem_( stem ) {
  CHECK( !stem_.empty() );
}

void BiomeDensityStatsCollector::collect( MapMatrix const& m ) {
  map_sz_ = m.size();
  CHECK(
      map_sz_.h == 70,
      "this stats collector requires the standard map size." );
  ++maps_total_;
  on_all_tiles(
      m, [&]( point const tile, MapSquare const& square ) {
        if( square.surface == e_surface::water ) return;
        ++land_total_;
        ++land_count_y_[tile.y + 1];
        ++biome_count_y_[tile.y + 1][square.ground];
      } );
}

void BiomeDensityStatsCollector::summarize() {}

void BiomeDensityStatsCollector::write() const {
  GnuPlotSettings const settings{
    .title   = format( "Biome Density (generated) ({}) [{}]",
                       stem_, maps_total_ ),
    .x_label = "Map Row (Y)",
    .y_label = "Density",
    .x_range = "1:70",
    .y_range = "0:0.7",
  };
  CsvData csv_data{ .header = { "y" }, .rows = {} };
  for( auto const gt : kGroundTypes )
    csv_data.header.push_back( base::to_str( gt ) );
  fs::path const kGeneratedDir =
      "tools/auto-measure/auto-map-gen/biomes/generated";
  for( int y = 0; y < map_sz_.h; ++y ) {
    vector<string> row{ /*y=*/to_string( y + 1 ) };
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
        value =
            double( *biome_count_y_row_gt ) / *land_count_y_row;
      }
    skip:
      row.push_back( to_string( value ) );
    }
    csv_data.rows.push_back( std::move( row ) );
  }
  generate_gnuplot( kGeneratedDir, stem_, settings, csv_data );
}

/****************************************************************
** WetnessStatsCollector
*****************************************************************/
struct WetnessStatsCollector : IMapStatsCollector {
  WetnessStatsCollector( string const& mode,
                         WeatherValue const climate )
    : mode_( mode ), climate_( climate ) {
    CHECK( !mode.empty() );
  }

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

  string generated_dir() const {
    return "tools/auto-measure/auto-map-gen/wetness/generated";
  }

  void gnuplot( GnuPlotSettings const settings,
                CsvData const& csv_data,
                string const& label ) const {
    generate_gnuplot( generated_dir(),
                      format( "{}.{}", mode_, label ), settings,
                      csv_data );
  }

 private:
  string const mode_;
  WeatherValue const climate_;
  size map_sz_         = {};
  int land_            = 0;
  double land_wetness_ = 0;
  int maps_            = 0;
  map<int /*row*/, int> land_by_row_;
  map<int /*col*/, int> land_by_col_;
  map<int /*row*/, enum_map<e_biome, int>>
      land_with_biome_by_row_;
  map<int /*col*/, enum_map<e_biome, int>>
      land_with_biome_by_col_;
  map<int /*row*/, enum_map<e_biome, double>>
      wetness_on_biome_by_row_;
  map<int /*col*/, enum_map<e_biome, double>>
      wetness_on_biome_by_col_;
  map<int /*row*/, double> wetness_by_row_;
  map<int /*col*/, double> wetness_by_col_;
};

void WetnessStatsCollector::collect( MapMatrix const& m ) {
  map_sz_ = m.size();
  ++maps_;

  matrix<double> const wetness = [&] {
    matrix<double> res;
    compute_wetness( m,
                     config_map_gen.terrain_generation.wetness,
                     climate_, res );
    CHECK_EQ( res.size(), m.size() );
    return res;
  }();

  on_all_tiles(
      m, [&]( point const tile, MapSquare const& center ) {
        using enum e_land_overlay;
        if( center.surface == e_surface::water ) return;
        ++land_;
        ++land_by_row_[tile.y];
        ++land_by_col_[tile.x];
        ++land_with_biome_by_row_[tile.y][center.ground];
        ++land_with_biome_by_col_[tile.x][center.ground];
        wetness_on_biome_by_row_[tile.y][center.ground] +=
            wetness[tile];
        wetness_on_biome_by_col_[tile.x][center.ground] +=
            wetness[tile];
        land_wetness_ += wetness[tile];
        wetness_by_row_[tile.y] += wetness[tile];
        wetness_by_col_[tile.x] += wetness[tile];
      } );
}

void WetnessStatsCollector::summarize() {}

void WetnessStatsCollector::write() const {
  using fmt::println;
  println( "land:         {}", double( land_ ) / maps_ );
  println( "land_wetness: {}", land_wetness_ / maps_ );

  {
    GnuPlotSettings const settings{
      .title   = format( "Wetness by Row (generated) ({}) [{}]",
                         mode_, maps_ ),
      .x_label = "Y (row)",
      .y_label = "Density",
      .x_range = "1:70",
      .y_range = "0:1",
    };

    CsvData csv_data{
      .header = { "y", "wetness" },
      .rows   = {},
    };

    for( int i = 2; i <= 69; ++i ) {
      vector<string> row{
        /*y=*/to_string( i ),
        /*wetness=*/"",
      };
      int const y     = i - 1;
      int const total = lookup( land_by_row_, y ).value_or( 0 );
      if( total > 0 )
        row[1] = to_string(
            lookup( wetness_by_row_, y ).value_or( 0 ) / total );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "wetness.rows" );
  }

  {
    GnuPlotSettings const settings{
      .title = format( "Wetness by Column (generated) ({}) [{}]",
                       mode_, maps_ ),
      .x_label = "X (column)",
      .y_label = "Density",
      .x_range = "1:56",
      .y_range = "0:1",
    };

    CsvData csv_data{
      .header = { "x", "wetness" },
      .rows   = {},
    };

    for( int i = 1; i <= 56; ++i ) {
      vector<string> row{
        /*y=*/to_string( i ),
        /*wetness=*/"",
      };
      int const x     = i - 1;
      int const total = lookup( land_by_col_, x ).value_or( 0 );
      if( total > 0 )
        row[1] = to_string(
            lookup( wetness_by_col_, x ).value_or( 0 ) / total );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "wetness.cols" );
  }

  {
    using enum e_terrain_formation;
    GnuPlotSettings const settings{
      .title = format(
          "Wetness on Biome by Row (generated) ({}) [{}]", mode_,
          maps_ ),
      .x_label = "Y",
      .y_label = "Wetness",
      .x_range = "1:70",
      .y_range = "0:1.0",
    };

    CsvData csv_data{
      .header = { "y" },
      .rows   = {},
    };

    for( e_biome const biome : kGroundTypes )
      csv_data.header.push_back( base::to_str( biome ) );
    for( int y = 0; y < map_sz_.h; ++y ) {
      vector<string> row{
        /*y=*/to_string( y + 1 ),
      };
      auto const row_wetness =
          lookup( wetness_on_biome_by_row_, y );
      auto const row_land = lookup( land_with_biome_by_row_, y );
      for( e_biome const biome : kGroundTypes ) {
        if( biome == e_biome::arctic ||
            !row_wetness.has_value() || !row_land.has_value() ||
            ( *row_land )[biome] == 0 ) {
          row.push_back( "0" );
          continue;
        }
        row.push_back( to_string( ( *row_wetness )[biome] /
                                  ( *row_land )[biome] ) );
      }
      csv_data.rows.push_back( std::move( row ) );
    }
    gnuplot( settings, csv_data, "biome.wetness.rows" );
  }

  {
    using enum e_terrain_formation;
    GnuPlotSettings const settings{
      .title = format(
          "Wetness on Biome by Column (generated) ({}) [{}]",
          mode_, maps_ ),
      .x_label = "X",
      .y_label = "Wetness",
      .x_range = "1:56",
      .y_range = "0:1.0",
    };

    CsvData csv_data{
      .header = { "x" },
      .rows   = {},
    };

    for( e_biome const biome : kGroundTypes )
      csv_data.header.push_back( base::to_str( biome ) );
    for( int x = 0; x < map_sz_.w; ++x ) {
      vector<string> row{
        /*x=*/to_string( x + 1 ),
      };
      auto const col_wetness =
          lookup( wetness_on_biome_by_col_, x );
      auto const col_land = lookup( land_with_biome_by_col_, x );
      for( e_biome const biome : kGroundTypes ) {
        if( biome == e_biome::arctic ||
            !col_wetness.has_value() || !col_land.has_value() ||
            ( *col_land )[biome] == 0 ) {
          row.push_back( "0" );
          continue;
        }
        row.push_back( to_string( ( *col_wetness )[biome] /
                                  ( *col_land )[biome] ) );
      }
      csv_data.rows.push_back( std::move( row ) );
    }
    gnuplot( settings, csv_data, "biome.wetness.cols" );
  }
}

/****************************************************************
** BiomeWetnessStatsCollector
*****************************************************************/
struct BiomeWetnessStatsCollector : IMapStatsCollector {
  BiomeWetnessStatsCollector( WeatherValue const climate )
    : climate_( climate ) {}

  static array<e_biome, 9> constexpr kWetOrdering = {
    e_biome::tundra,    e_biome::desert, e_biome::savannah,
    e_biome::plains,    e_biome::arctic, e_biome::prairie,
    e_biome::grassland, e_biome::swamp,  e_biome::marsh,
  };

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

 private:
  WeatherValue const climate_;
  size map_sz_         = {};
  int land_            = 0;
  double land_wetness_ = 0;
  refl::enum_map<e_biome, double> biome_wetness_;
  refl::enum_map<e_biome, int> biome_count_;
  int maps_total_ = 0;
};

void BiomeWetnessStatsCollector::collect( MapMatrix const& m ) {
  map_sz_ = m.size();
  ++maps_total_;

  matrix<double> const wetness = [&] {
    matrix<double> res;
    compute_wetness( m,
                     config_map_gen.terrain_generation.wetness,
                     climate_, res );
    CHECK_EQ( res.size(), m.size() );
    return res;
  }();

  on_all_tiles(
      m, [&]( point const tile, MapSquare const& center ) {
        if( center.surface == e_surface::water ) return;
        ++biome_count_[center.ground];
        biome_wetness_[center.ground] += wetness[tile];
        land_wetness_ += wetness[tile];
        ++land_;
      } );
}

void BiomeWetnessStatsCollector::summarize() {}

void BiomeWetnessStatsCollector::write() const {
  enum_map<e_biome, double> const& kTargets{
    // clang-format off
    // bbmm
    // {e_biome::tundra,    0.097},
    // {e_biome::desert,    0.187},
    // {e_biome::savannah,  0.209},
    // {e_biome::plains,    0.283},
    // {e_biome::prairie,   0.442},
    // {e_biome::grassland, 0.477},
    // {e_biome::swamp,     0.580},
    // {e_biome::marsh,     0.614},
    // mmmm
    {e_biome::tundra,    0.140},
    {e_biome::desert,    0.267},
    {e_biome::savannah,  0.307},
    {e_biome::plains,    0.364},
    {e_biome::prairie,   0.546},
    {e_biome::grassland, 0.602},
    {e_biome::swamp,     0.689},
    {e_biome::marsh,     0.715},
    // clang-format on
  };
  fmt::println( "land:         {}",
                double( land_ ) / maps_total_ );
  for( e_biome const biome : enum_values<e_biome> )
    fmt::println( "{:<12}:       {}", biome,
                  double( biome_count_[biome] ) / maps_total_ );
  fmt::println( "land_wetness: {}",
                land_wetness_ / maps_total_ );
  fmt::println( "" );
  fmt::println( "{:12}: {:>12} {:>12} {:>12}", "biome",
                "wetness", "target", "ratio" );
  fmt::println(
      "-----------------------------------------------------" );
  double error_abs = 0;
  for( e_biome const biome : kWetOrdering ) {
    if( biome == e_biome::arctic ) continue;
    double const wetness =
        biome_wetness_[biome] / biome_count_[biome];
    fmt::println( "{:12}: {:12.3f} {:12.3f} {:12.3f}", biome,
                  wetness, kTargets[biome],
                  wetness / kTargets[biome] );
    error_abs += abs( wetness / kTargets[biome] - 1 );
  }
  double const error = error_abs / 8;
  fmt::println(
      "-----------------------------------------------------" );
  fmt::println( "{:12}: {:>12} {:>12} {:12.3f}", "error", "", "",
                error );
}

/****************************************************************
** FormationsStatsCollector
*****************************************************************/
struct FormationsStatsCollector : IMapStatsCollector {
  FormationsStatsCollector( string const& mode );

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

  void assign_segment( MapMatrix const& m,
                       e_terrain_formation const formation,
                       point const start, int const segment );

  point find_center( int segment, e_terrain_formation kind );

  void find_connected( MapMatrix const& m,
                       e_terrain_formation formation );

  [[nodiscard]] bool is_large_range( int const length );

  [[nodiscard]] bool ocean_adjacent( MapMatrix const& m,
                                     point const p );

  [[nodiscard]] bool ocean_adjacent_cardinal( MapMatrix const& m,
                                              point const p );

  string generated_dir() const {
    return "tools/auto-measure/auto-map-gen/overlays/generated";
  }

  string generated_file_path( string const& filename ) const {
    return format( "{}/{}", generated_dir(), filename );
  }

  string generated_json_path( string const& label ) const {
    return generated_file_path(
        format( "{}.{}.json", mode_, label ) );
  }

  void gnuplot( GnuPlotSettings const settings,
                CsvData const& csv_data,
                string const& label ) const {
    generate_gnuplot( generated_dir(),
                      format( "{}.{}", mode_, label ), settings,
                      csv_data );
  }

 private:
  void emit_data_file() const;
  void emit_inputs_file() const;

  inline static auto const FORMATION_ORDER_CLEARING = {
    e_terrain_formation::mountains, //
    e_terrain_formation::hills,     //
    e_terrain_formation::clearing,  //
  };

  inline static auto const BIOME_ORDERING = {
    e_biome::savannah,  //
    e_biome::grassland, //
    e_biome::tundra,    //
    e_biome::plains,    //
    e_biome::prairie,   //
    e_biome::desert,    //
    e_biome::swamp,     //
    e_biome::marsh,     //
    e_biome::arctic,    //
  };

  string const mode_;
  size map_sz_ = {};
  int land_    = 0;
  int maps_    = 0;
  int tiles_   = 0;

  int land_arctic_row_              = 0;
  int land_ocean_adjacent_          = 0;
  int land_ocean_adjacent_cardinal_ = 0;
  int land_non_mounds_              = 0;

  template<typename T>
  using M = enum_map<e_terrain_formation, T>;

  map<int /*row*/, int> land_by_row_;
  map<int /*col*/, int> land_by_col_;
  enum_map<e_biome, int> land_with_biome_;
  map<int /*y*/, enum_map<e_biome, int>> land_with_biome_by_row_;
  map<int /*x*/, enum_map<e_biome, int>> land_with_biome_by_col_;
  M<int> count_;
  int count_forest_ = 0;
  M<int> count_squared_;
  int count_rivers_on_land_ = 0;
  M<int> count_arctic_rows_;
  M<enum_map<e_biome, int>> count_with_biome_;
  M<map<int /*y*/, enum_map<e_biome, int>>>
      count_with_biome_by_row_;
  M<map<int /*x*/, enum_map<e_biome, int>>>
      count_with_biome_by_col_;
  M<map<int /*row*/, int>> count_by_row_;
  M<map<int /*col*/, int>> count_by_col_;
  // Note this includes singletons.
  M<int> num_ranges_;
  M<map<int /*length*/, int>> count_range_length_;
  M<int> count_range_centers_;
  M<map<int /*row*/, int>> count_range_centers_by_row_;
  M<map<int /*col*/, int>> count_range_centers_by_col_;
  M<int> count_ocean_adjacent_;
  M<int> count_ocean_adjacent_cardinal_;
  M<int> count_large_range_;
  // Count of tiles that are part of a large range that are adja-
  // cent to water.
  M<map<int /*row*/, int>> count_large_range_by_row_;
  // Count of tiles that are part of a large range that are adja-
  // cent to water.
  M<map<int /*col*/, int>> count_large_range_by_col_;
  // Count of tiles that are part of a large range that are adja-
  // cent to water.
  M<int> count_large_range_ocean_adjacent_;
  M<int> count_large_range_ocean_adjacent_cardinal_;
  int count_mountains_adjacent_to_hills_ = 0;
  int count_hills_adjacent_to_mountains_ = 0;
  struct PerMap {
    // This per-map count is kept so that we can compute std.
    // devi- ation of the densities.
    M<int> count;
    int count_forest = 0;
    M<int> segments;
    M<map<point /*tile*/, int>> tile_to_segment;
    M<map<int /*segment*/, int>> segment_to_length;
    M<map<int /*segment*/, vector<point>>> segment_to_tiles;
    // Note this includes singletons.
    M<int> num_ranges;
  };
  PerMap per_map_;
};

FormationsStatsCollector::FormationsStatsCollector(
    string const& mode )
  : mode_( mode ) {
  CHECK( !mode.empty() );
}

bool FormationsStatsCollector::is_large_range(
    int const length ) {
  return length >= 5;
}

bool FormationsStatsCollector::ocean_adjacent(
    MapMatrix const& m, point const p ) {
  bool has = false;
  on_surrounding( m, p, [&]( point, MapSquare const& adjacent ) {
    if( adjacent.surface == e_surface::water ) has = true;
  } );
  return has;
}

bool FormationsStatsCollector::ocean_adjacent_cardinal(
    MapMatrix const& m, point const p ) {
  bool has = false;
  on_surrounding_cardinal(
      m, p,
      [&]( point, MapSquare const& adjacent,
           e_cardinal_direction ) {
        if( adjacent.surface == e_surface::water ) has = true;
      } );
  return has;
}

void FormationsStatsCollector::assign_segment(
    MapMatrix const& m, e_terrain_formation const kind,
    point const start, int const segment ) {
  using enum e_surface;
  per_map_.tile_to_segment[kind][start] = segment;
  per_map_.segment_to_tiles[kind][segment].push_back( start );
  CHECK( m[start].surface != e_surface::water );
  on_surrounding_cardinal(
      m, start,
      [&]( point const p, MapSquare const& adjacent,
           e_cardinal_direction ) {
        bool const is_arctic_row =
            p.y == 0 || p.y == map_sz_.h - 1;
        if( is_arctic_row ) return;
        if( adjacent.surface == e_surface::water ) return;
        if( per_map_.tile_to_segment[kind].contains( p ) &&
            per_map_.tile_to_segment[kind][p] > 0 ) {
          CHECK( terrain_formation_for( adjacent ) == kind );
          CHECK( per_map_.tile_to_segment[kind][p] == segment );
          return;
        }
        if( terrain_formation_for( adjacent ) == kind ) {
          bool const is_surrounding_water =
              m[p].surface == e_surface::water;
          CHECK( !is_surrounding_water );
          assign_segment( m, kind, p, segment );
        }
      } );
}

point FormationsStatsCollector::find_center(
    int const segment, e_terrain_formation const kind ) {
  CHECK( per_map_.segment_to_tiles[kind].contains( segment ) );
  auto const& tiles = per_map_.segment_to_tiles[kind][segment];
  CHECK( !tiles.empty() );
  int x = 0;
  int y = 0;
  for( point const p : tiles ) {
    x = x + p.x;
    y = y + p.y;
  }
  x = static_cast<int>( floor( double( x ) / tiles.size() ) );
  y = static_cast<int>( floor( double( y ) / tiles.size() ) );
  return { .x = x, .y = y };
}

void FormationsStatsCollector::find_connected(
    MapMatrix const& m, e_terrain_formation const kind ) {
  for( auto const& [tile, segment] :
       per_map_.tile_to_segment[kind] ) {
    CHECK( terrain_formation_for( m[tile] ) == kind );
    if( segment == 0 ) {
      ++per_map_.segments[kind];
      assign_segment( m, kind, tile, per_map_.segments[kind] );
    }
  }

  map<int /*segment*/, int /*length*/> segment_length;
  on_all_tiles(
      m, [&]( point const tile, MapSquare const& center ) {
        if( center.surface == e_surface::water ) return;
        bool const is_arctic_row =
            tile.y == 0 || tile.y == map_sz_.h - 1;
        if( is_arctic_row ) return;
        if( terrain_formation_for( center ) != kind ) return;
        int const segment = per_map_.tile_to_segment[kind][tile];
        CHECK_GT( segment, 0 );
        ++segment_length[segment];
      } );

  int max_length = 0;
  for( int segment = 1; segment <= per_map_.segments[kind];
       ++segment ) {
    int const length = segment_length[segment];
    CHECK_GT( length, 0 );
    max_length = std::max( max_length, length );
    ++count_range_length_[kind][length];
    per_map_.segment_to_length[kind][segment] = length;
    ++count_range_centers_[kind];
    point const center = find_center( segment, kind );
    ++count_range_centers_by_row_[kind][center.y];
    ++count_range_centers_by_col_[kind][center.x];
  }

  num_ranges_[kind] += per_map_.segments[kind];
  per_map_.num_ranges[kind] = per_map_.segments[kind];

  on_all_tiles( m, [&]( point const tile,
                        MapSquare const& center ) {
    bool const is_arctic_row =
        tile.y == 0 || tile.y == map_sz_.h - 1;
    if( is_arctic_row ) return;
    if( center.surface == e_surface::water ) return;
    if( !per_map_.tile_to_segment[kind].contains( tile ) )
      return;
    int const segment = per_map_.tile_to_segment[kind][tile];
    CHECK( segment > 0 );
    int const length = per_map_.segment_to_length[kind][segment];
    if( !is_large_range( length ) ) return;
    // We have a tile on a large range.
    ++count_large_range_[kind];
    ++count_large_range_by_row_[kind][tile.y];
    ++count_large_range_by_col_[kind][tile.x];
    bool const has_ocean_adjacent = ocean_adjacent( m, tile );
    bool const has_ocean_adjacent_cardinal =
        ocean_adjacent_cardinal( m, tile );
    if( has_ocean_adjacent )
      ++count_large_range_ocean_adjacent_[kind];
    if( has_ocean_adjacent_cardinal )
      ++count_large_range_ocean_adjacent_cardinal_[kind];
  } );
}

void FormationsStatsCollector::collect( MapMatrix const& m ) {
  per_map_ = {};
  map_sz_  = m.size();
  ++maps_;

  using enum e_terrain_formation;

  on_all_tiles( m, [&]( point const tile,
                        MapSquare const& center ) {
    ++tiles_;
    auto const has_mountains = [&]( MapSquare const& square ) {
      return square.overlay == e_land_overlay::mountains;
    };
    auto const has_hills = [&]( MapSquare const& square ) {
      return square.overlay == e_land_overlay::hills;
    };
    auto const has_clearing = [&]( MapSquare const& square ) {
      return square.surface == e_surface::land &&
             square.overlay == nothing;
    };
    auto const has_forest = [&]( MapSquare const& square ) {
      return square.overlay == e_land_overlay::forest;
    };
    auto const has_river = [&]( MapSquare const& square ) {
      return square.river.has_value();
    };

    if( center.surface == e_surface::water ) return;
    ++land_;
    bool const is_arctic_row =
        tile.y == 0 || tile.y == map_sz_.h - 1;

    if( is_arctic_row ) {
      ++land_arctic_row_;
      if( has_mountains( center ) )
        ++count_arctic_rows_[mountains];
      if( has_hills( center ) ) ++count_arctic_rows_[hills];
      if( has_clearing( center ) )
        ++count_arctic_rows_[clearing];
      return;
    }

    bool const has_ocean_adjacent = ocean_adjacent( m, tile );
    bool const has_ocean_adjacent_cardinal =
        ocean_adjacent_cardinal( m, tile );

    ++land_by_row_[tile.y];
    ++land_by_col_[tile.x];
    if( has_ocean_adjacent ) ++land_ocean_adjacent_;
    if( has_ocean_adjacent_cardinal )
      ++land_ocean_adjacent_cardinal_;
    ++land_with_biome_[center.ground];
    ++land_with_biome_by_row_[tile.y][center.ground];
    ++land_with_biome_by_col_[tile.x][center.ground];

    bool has_hills_adjacent     = false;
    bool has_mountains_adjacent = false;
    on_surrounding_cardinal(
        m, tile,
        [&]( point const, MapSquare const& adjacent,
             e_cardinal_direction const ) {
          if( adjacent.surface == e_surface::water ) return;
          if( is_arctic_row ) return;
          if( has_mountains( adjacent ) )
            has_mountains_adjacent = true;
          if( has_hills( adjacent ) ) has_hills_adjacent = true;
        } );
    if( has_river( center ) ) ++count_rivers_on_land_;

    if( has_mountains( center ) ) {
      ++count_[mountains];
      ++per_map_.count[mountains];
      ++count_by_row_[mountains][tile.y];
      ++count_by_col_[mountains][tile.x];
      if( has_ocean_adjacent )
        ++count_ocean_adjacent_[mountains];
      if( has_ocean_adjacent_cardinal )
        ++count_ocean_adjacent_cardinal_[mountains];
      if( has_hills_adjacent )
        ++count_mountains_adjacent_to_hills_;
      per_map_.tile_to_segment[mountains][tile] = 0;
      ++count_with_biome_[mountains][center.ground];
      ++count_with_biome_by_row_[mountains][tile.y]
                                [center.ground];
      ++count_with_biome_by_col_[mountains][tile.x]
                                [center.ground];
    }

    if( has_hills( center ) ) {
      ++count_[hills];
      ++per_map_.count[hills];
      ++count_by_row_[hills][tile.y];
      ++count_by_col_[hills][tile.x];
      if( has_ocean_adjacent ) ++count_ocean_adjacent_[hills];
      if( has_ocean_adjacent_cardinal )
        ++count_ocean_adjacent_cardinal_[hills];
      if( has_mountains_adjacent )
        ++count_hills_adjacent_to_mountains_;
      per_map_.tile_to_segment[hills][tile] = 0;
      ++count_with_biome_[hills][center.ground];
      ++count_with_biome_by_row_[hills][tile.y][center.ground];
      ++count_with_biome_by_col_[hills][tile.x][center.ground];
    }

    if( has_forest( center ) ) {
      ++count_forest_;
      ++per_map_.count_forest;
    }

    bool const has_overlays = has_mountains( center ) ||
                              has_hills( center ) ||
                              has_forest( center );
    CHECK( has_clearing( center ) == !has_overlays );

    if( has_clearing( center ) ) {
      ++count_[clearing];
      ++per_map_.count[clearing];
      ++count_by_row_[clearing][tile.y];
      ++count_by_col_[clearing][tile.x];
      if( has_ocean_adjacent ) ++count_ocean_adjacent_[clearing];
      if( has_ocean_adjacent_cardinal )
        ++count_ocean_adjacent_cardinal_[clearing];
      per_map_.tile_to_segment[clearing][tile] = 0;
      ++count_with_biome_[clearing][center.ground];
      ++count_with_biome_by_row_[clearing][tile.y]
                                [center.ground];
      ++count_with_biome_by_col_[clearing][tile.x]
                                [center.ground];
    }

    if( !has_mountains( center ) && !has_hills( center ) )
      ++land_non_mounds_;
  } );

  find_connected( m, mountains );
  find_connected( m, hills );
  find_connected( m, clearing );

  count_squared_[mountains] +=
      per_map_.count[mountains] * per_map_.count[mountains];
  count_squared_[hills] +=
      per_map_.count[hills] * per_map_.count[hills];
  count_squared_[clearing] +=
      per_map_.count[clearing] * per_map_.count[clearing];
}

void FormationsStatsCollector::summarize() {}

void FormationsStatsCollector::emit_data_file() const {
  using enum e_terrain_formation;
  using namespace cdr::literals;
  using cdr::list;
  using cdr::table;

  double const maps = maps_;
  double const land = land_;

  auto const stddev = []( double const s1, double const s2,
                          double const denominator ) {
    return sqrt( s2 / denominator - pow( s1 / denominator, 2 ) );
  };

  list const DATA_KEY_ORDER = {
    "savs",                                        //
    "tiles",                                       //
    "land",                                        //
    "land_arctic_row",                             //
    "land_ocean_adjacent",                         //
    "land_ocean_adjacent_cardinal",                //
    "land_non_mounds",                             //
    "count",                                       //
    "count_forest",                                //
    "stddev",                                      //
    "density",                                     //
    "forest_density_non_mounds",                   //
    "count_hills_plus_land_rivers",                //
    "density_hills_plus_land_rivers",              //
    "count_arctic_rows",                           //
    "num_ranges",                                  //
    "num_ranges_1",                                //
    "num_ranges_1_per_land",                       //
    "num_ranges_per_land",                         //
    "count_range_centers",                         //
    "density_range_centers",                       //
    "count_ocean_adjacent",                        //
    "count_ocean_adjacent_cardinal",               //
    "density_ocean_adjacent",                      //
    "density_ocean_adjacent_cardinal",             //
    "count_large_range",                           //
    "count_large_range_ocean_adjacent",            //
    "count_large_range_ocean_adjacent_cardinal",   //
    "density_large_range",                         //
    "density_large_range_ocean_adjacent",          //
    "density_large_range_ocean_adjacent_cardinal", //
    "count_mountains_adjacent_to_hills",           //
    "count_hills_adjacent_to_mountains",           //
    "land_with_biome",                             //
    "count_with_biome",                            //
    "density_on_biome",                            //
  };

  list const FORMATION_ORDER_CLEARING_CDR = {
    "mountains", //
    "hills",     //
    "clearing",  //
  };

  list const BIOME_ORDERING_CDR = {
    "savannah",  //
    "grassland", //
    "tundra",    //
    "plains",    //
    "prairie",   //
    "desert",    //
    "swamp",     //
    "marsh",     //
    "arctic",    //
  };

  table o;
  o["__key_order"]         = DATA_KEY_ORDER;
  o["savs"]                = int( maps );
  o["tiles"]               = tiles_;
  o["land"]                = land_ / maps;
  o["land_arctic_row"]     = land_arctic_row_ / maps;
  o["land_ocean_adjacent"] = land_ocean_adjacent_ / maps;
  o["land_ocean_adjacent_cardinal"] =
      land_ocean_adjacent_cardinal_ / maps;
  o["land_non_mounds"] = land_non_mounds_ / maps;
  o["count"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["count"]["mountains"] = count_[mountains] / maps;
  o["count"]["hills"]     = count_[hills] / maps;
  o["count"]["clearing"]  = count_[clearing] / maps;
  o["count_forest"]       = count_forest_ / maps;
  o["stddev"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["stddev"]["mountains"] = stddev(
      count_[mountains], count_squared_[mountains], maps );
  o["stddev"]["hills"] =
      stddev( count_[hills], count_squared_[hills], maps );
  o["stddev"]["clearing"] =
      stddev( count_[clearing], count_squared_[clearing], maps );
  o["density"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["density"]["mountains"] = count_[mountains] / land;
  o["density"]["hills"]     = count_[hills] / land;
  o["density"]["clearing"]  = count_[clearing] / land;
  o["forest_density_non_mounds"] =
      count_forest_ / double( land_non_mounds_ );
  o["count_hills_plus_land_rivers"] =
      count_rivers_on_land_ + count_[hills];
  o["density_hills_plus_land_rivers"] =
      ( count_rivers_on_land_ + count_[hills] ) / land;
  o["count_arctic_rows"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["count_arctic_rows"]["mountains"] =
      count_arctic_rows_[mountains] / maps;
  o["count_arctic_rows"]["hills"] =
      count_arctic_rows_[hills] / maps;
  o["count_arctic_rows"]["clearing"] =
      count_arctic_rows_[clearing] / maps;
  o["num_ranges"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["num_ranges"]["mountains"] = num_ranges_[mountains] / maps;
  o["num_ranges"]["hills"]     = num_ranges_[hills] / maps;
  o["num_ranges"]["clearing"]  = num_ranges_[clearing] / maps;
  o["num_ranges_1"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["num_ranges_1"]["mountains"] =
      lookup( count_range_length_[mountains], 1 ).value_or( 0 ) /
      maps;
  o["num_ranges_1"]["hills"] =
      lookup( count_range_length_[hills], 1 ).value_or( 0 ) /
      maps;
  o["num_ranges_1"]["clearing"] =
      lookup( count_range_length_[clearing], 1 ).value_or( 0 ) /
      maps;
  o["num_ranges_1_per_land"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["num_ranges_1_per_land"]["mountains"] =
      lookup( count_range_length_[mountains], 1 ).value_or( 0 ) /
      land;
  o["num_ranges_1_per_land"]["hills"] =
      lookup( count_range_length_[hills], 1 ).value_or( 0 ) /
      land;
  o["num_ranges_1_per_land"]["clearing"] =
      lookup( count_range_length_[clearing], 1 ).value_or( 0 ) /
      land;
  o["num_ranges_per_land"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["num_ranges_per_land"]["mountains"] =
      num_ranges_[mountains] / land;
  o["num_ranges_per_land"]["hills"] = num_ranges_[hills] / land;
  o["num_ranges_per_land"]["clearing"] =
      num_ranges_[clearing] / land;
  o["count_range_centers"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["count_range_centers"]["mountains"] =
      count_range_centers_[mountains] / maps;
  o["count_range_centers"]["hills"] =
      count_range_centers_[hills] / maps;
  o["count_range_centers"]["clearing"] =
      count_range_centers_[clearing] / maps;
  o["density_range_centers"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["density_range_centers"]["mountains"] =
      count_range_centers_[mountains] / land;
  o["density_range_centers"]["hills"] =
      count_range_centers_[hills] / land;
  o["density_range_centers"]["clearing"] =
      count_range_centers_[clearing] / land;
  o["count_ocean_adjacent"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["count_ocean_adjacent"]["mountains"] =
      count_ocean_adjacent_[mountains] / maps;
  o["count_ocean_adjacent"]["hills"] =
      count_ocean_adjacent_[hills] / maps;
  o["count_ocean_adjacent"]["clearing"] =
      count_ocean_adjacent_[clearing] / maps;
  o["count_ocean_adjacent_cardinal"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["count_ocean_adjacent_cardinal"]["mountains"] =
      count_ocean_adjacent_cardinal_[mountains] / maps;
  o["count_ocean_adjacent_cardinal"]["hills"] =
      count_ocean_adjacent_cardinal_[hills] / maps;
  o["count_ocean_adjacent_cardinal"]["clearing"] =
      count_ocean_adjacent_cardinal_[clearing] / maps;
  o["density_ocean_adjacent"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["density_ocean_adjacent"]["mountains"] =
      count_ocean_adjacent_[mountains] /
      double( land_ocean_adjacent_ );
  o["density_ocean_adjacent"]["hills"] =
      count_ocean_adjacent_[hills] /
      double( land_ocean_adjacent_ );
  o["density_ocean_adjacent"]["clearing"] =
      count_ocean_adjacent_[clearing] /
      double( land_ocean_adjacent_ );
  o["density_ocean_adjacent_cardinal"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["density_ocean_adjacent_cardinal"]["mountains"] =
      count_ocean_adjacent_cardinal_[mountains] /
      double( land_ocean_adjacent_cardinal_ );
  o["density_ocean_adjacent_cardinal"]["hills"] =
      count_ocean_adjacent_cardinal_[hills] /
      double( land_ocean_adjacent_cardinal_ );
  o["density_ocean_adjacent_cardinal"]["clearing"] =
      count_ocean_adjacent_cardinal_[clearing] /
      double( land_ocean_adjacent_cardinal_ );
  o["count_large_range"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["count_large_range"]["mountains"] =
      count_large_range_[mountains] / maps;
  o["count_large_range"]["hills"] =
      count_large_range_[hills] / maps;
  o["count_large_range"]["clearing"] =
      count_large_range_[clearing] / maps;
  o["count_large_range_ocean_adjacent"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["count_large_range_ocean_adjacent"]["mountains"] =
      count_large_range_ocean_adjacent_[mountains] / maps;
  o["count_large_range_ocean_adjacent"]["hills"] =
      count_large_range_ocean_adjacent_[hills] / maps;
  o["count_large_range_ocean_adjacent"]["clearing"] =
      count_large_range_ocean_adjacent_[clearing] / maps;
  o["count_large_range_ocean_adjacent_cardinal"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["count_large_range_ocean_adjacent_cardinal"]["mountains"] =
      count_large_range_ocean_adjacent_cardinal_[mountains] /
      maps;
  o["count_large_range_ocean_adjacent_cardinal"]["hills"] =
      count_large_range_ocean_adjacent_cardinal_[hills] / maps;
  o["count_large_range_ocean_adjacent_cardinal"]["clearing"] =
      count_large_range_ocean_adjacent_cardinal_[clearing] /
      maps;
  o["density_large_range"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  o["density_large_range"]["mountains"] =
      count_large_range_[mountains] / land;
  o["density_large_range"]["hills"] =
      count_large_range_[hills] / land;
  o["density_large_range"]["clearing"] =
      count_large_range_[clearing] / land;
  o["density_large_range_ocean_adjacent"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["density_large_range_ocean_adjacent"]["mountains"] =
      count_large_range_ocean_adjacent_[mountains] /
      double( land_ocean_adjacent_ );
  o["density_large_range_ocean_adjacent"]["hills"] =
      count_large_range_ocean_adjacent_[hills] /
      double( land_ocean_adjacent_ );
  o["density_large_range_ocean_adjacent"]["clearing"] =
      count_large_range_ocean_adjacent_[clearing] /
      double( land_ocean_adjacent_ );
  o["density_large_range_ocean_adjacent_cardinal"] = table{
    "__key_order"_key = FORMATION_ORDER_CLEARING_CDR,
  };
  o["density_large_range_ocean_adjacent_cardinal"]["mountains"] =
      count_large_range_ocean_adjacent_cardinal_[mountains] /
      double( land_ocean_adjacent_cardinal_ );
  o["density_large_range_ocean_adjacent_cardinal"]["hills"] =
      count_large_range_ocean_adjacent_cardinal_[hills] /
      double( land_ocean_adjacent_cardinal_ );
  o["density_large_range_ocean_adjacent_cardinal"]["clearing"] =
      count_large_range_ocean_adjacent_cardinal_[clearing] /
      double( land_ocean_adjacent_cardinal_ );
  o["count_mountains_adjacent_to_hills"] =
      count_mountains_adjacent_to_hills_ / maps;
  o["count_hills_adjacent_to_mountains"] =
      count_hills_adjacent_to_mountains_ / maps;

  o["land_with_biome"] =
      table{ "__key_order"_key = BIOME_ORDERING_CDR };
  for( e_biome const biome : BIOME_ORDERING ) {
    auto const& biome_str           = base::to_str( biome );
    o["land_with_biome"][biome_str] = land_with_biome_[biome];
  }

  o["count_with_biome"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  for( e_terrain_formation const kind :
       FORMATION_ORDER_CLEARING ) {
    auto const& kind_str = base::to_str( kind );
    o["count_with_biome"][kind_str] =
        table{ "__key_order"_key = BIOME_ORDERING_CDR };
    for( e_biome const biome : BIOME_ORDERING ) {
      auto const& biome_str = base::to_str( biome );
      o["count_with_biome"][kind_str][biome_str] =
          count_with_biome_[kind][biome];
    }
  }

  o["density_on_biome"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  for( e_terrain_formation const kind :
       FORMATION_ORDER_CLEARING ) {
    auto const& kind_str = base::to_str( kind );
    o["density_on_biome"][kind_str] =
        table{ "__key_order"_key = BIOME_ORDERING_CDR };
    for( e_biome const biome : BIOME_ORDERING ) {
      auto const& biome_str = base::to_str( biome );
      if( land_with_biome_[biome] > 0 )
        o["density_on_biome"][kind_str][biome_str] =
            count_with_biome_[kind][biome] /
            double( land_with_biome_[biome] );
      else
        o["density_on_biome"][kind_str][biome_str] = 0;
    }
  }

  string const out_filename = generated_json_path( "data" );
  ofstream out( out_filename );
  CHECK( out.good() );
  out << rcl::emit_json( o, rcl::JsonEmitOptions{
                              .key_order_tag = "__key_order" } );
}

void FormationsStatsCollector::emit_inputs_file() const {
  using enum e_terrain_formation;
  using namespace cdr::literals;
  using cdr::list;
  using cdr::table;

  list const DATA_KEY_ORDER = {
    "density_on_biome", //
  };

  list const FORMATION_ORDER_CLEARING_CDR = {
    "mountains", //
    "hills",     //
    "clearing",  //
  };

  list const BIOME_ORDERING_CDR = {
    "savannah",  //
    "grassland", //
    "tundra",    //
    "plains",    //
    "prairie",   //
    "desert",    //
    "swamp",     //
    "marsh",     //
    "arctic",    //
  };

  table o;
  o["__key_order"] = DATA_KEY_ORDER;

  o["density_on_biome"] =
      table{ "__key_order"_key = FORMATION_ORDER_CLEARING_CDR };
  for( e_terrain_formation const kind :
       FORMATION_ORDER_CLEARING ) {
    auto const& kind_str = base::to_str( kind );
    o["density_on_biome"][kind_str] =
        table{ "__key_order"_key = BIOME_ORDERING_CDR };
    for( e_biome const biome : BIOME_ORDERING ) {
      auto const& biome_str = base::to_str( biome );
      o["density_on_biome"][kind_str][biome_str] =
          config_map_gen.terrain_generation.formations
              .formation[kind]
              .biome_density[biome]
              .probability;
    }
  }

  string const out_filename = generated_json_path( "inputs" );
  ofstream out( out_filename );
  CHECK( out.good() );
  out << rcl::emit_json( o, rcl::JsonEmitOptions{
                              .key_order_tag = "__key_order" } );
}

void FormationsStatsCollector::write() const {
  using enum e_terrain_formation;

  emit_data_file();
  emit_inputs_file();

  double const maps = maps_;

  {
    GnuPlotSettings const settings{
      .title =
          format( "Range Length Histogram (generated) ({}) [{}]",
                  mode_, maps_ ),
      .x_label = "Length (cardinal adjacent)",
      .y_label = "Frequency",
      .x_range = "1:30",
      .y_range = "-20:0",
    };

    CsvData csv_data{
      .header = { "length", "mountains", "hills", "clearing",
                  "fit" },
      .rows   = {},
    };

    int max_length   = 0;
    int total_ranges = 0;
    for( e_terrain_formation const kind :
         FORMATION_ORDER_CLEARING ) {
      total_ranges += num_ranges_[kind];
      for( auto const& [length, count] :
           count_range_length_[kind] )
        if( count > 0 )
          max_length = std::max( max_length, length );
    }
    M<double> length_1_val;
    length_1_val[mountains] = 1;
    length_1_val[hills]     = 1;
    length_1_val[clearing]  = 1;
    for( int i = 1; i <= max_length; ++i ) {
      vector<string> row{
        /*length=*/to_string( i ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
        /*fit=*/"",
      };
      double mountains_value =
          log( lookup( count_range_length_[mountains], i )
                   .value_or( 1 ) /
               double( total_ranges ) );
      double hills_value = log(
          lookup( count_range_length_[hills], i ).value_or( 1 ) /
          double( total_ranges ) );
      double clearing_value =
          log( lookup( count_range_length_[clearing], i )
                   .value_or( 1 ) /
               double( total_ranges ) );
      double const fit_value = 0; //-2.0 * ( i - 1 );
      if( i == 1 ) {
        length_1_val[mountains] = mountains_value;
        length_1_val[hills]     = hills_value;
        length_1_val[clearing]  = clearing_value;
      }
      mountains_value =
          mountains_value - length_1_val[mountains];
      hills_value    = hills_value - length_1_val[hills];
      clearing_value = clearing_value - length_1_val[clearing];
      row[1]         = to_string( mountains_value );
      row[2]         = to_string( hills_value );
      row[3]         = to_string( clearing_value );
      row[4]         = to_string( fit_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "lengths" );
  }

  {
    GnuPlotSettings const settings{
      .title =
          format( "Row Based Densities (generated) ({}) [{}]",
                  mode_, maps_ ),
      .x_label = "Y (row)",
      .y_label = "Density",
      .x_range = "1:70",
      .y_range = "0:.2",
    };

    CsvData csv_data{
      .header = { "y", "mountains", "hills", "clearing" },
      .rows   = {},
    };

    for( int y = 1; y < map_sz_.h - 1; ++y ) {
      vector<string> row{
        /*y=*/to_string( y + 1 ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
      };
      double const mountains_value =
          lookup( count_by_row_[mountains], y ).value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      double const hills_value =
          lookup( count_by_row_[hills], y ).value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      double const clearing_value =
          lookup( count_by_row_[clearing], y ).value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      row[1] = to_string( mountains_value );
      row[2] = to_string( hills_value );
      row[3] = to_string( clearing_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "rows.any" );
  }

  {
    GnuPlotSettings const settings{
      .title =
          format( "Column Based Densities (generated) ({}) [{}]",
                  mode_, maps_ ),
      .x_label = "X (column)",
      .y_label = "Density",
      .x_range = "1:56",
      .y_range = "0:.2",
    };

    CsvData csv_data{
      .header = { "x", "mountains", "hills", "clearing" },
      .rows   = {},
    };

    for( int x = 0; x < map_sz_.w; ++x ) {
      vector<string> row{
        /*x=*/to_string( x + 1 ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
      };
      double const mountains_value =
          lookup( count_by_col_[mountains], x ).value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      double const hills_value =
          lookup( count_by_col_[hills], x ).value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      double const clearing_value =
          lookup( count_by_col_[clearing], x ).value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      row[1] = to_string( mountains_value );
      row[2] = to_string( hills_value );
      row[3] = to_string( clearing_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "cols.any" );
  }

  {
    GnuPlotSettings const settings{
      .title   = format( "Row Based Range Center Densities "
                           "(generated) ({}) [{}]",
                         mode_, maps_ ),
      .x_label = "Y (row)",
      .y_label = "Density",
      .x_range = "1:70",
      .y_range = "0:.1",
    };

    CsvData csv_data{
      .header = { "y", "mountains", "hills", "clearing" },
      .rows   = {},
    };

    for( int y = 1; y < map_sz_.h - 1; ++y ) {
      vector<string> row{
        /*y=*/to_string( y + 1 ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
      };
      double const mountains_value =
          lookup( count_range_centers_by_row_[mountains], y )
              .value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      double const hills_value =
          lookup( count_range_centers_by_row_[hills], y )
              .value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      double const clearing_value =
          lookup( count_range_centers_by_row_[clearing], y )
              .value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      row[1] = to_string( mountains_value );
      row[2] = to_string( hills_value );
      row[3] = to_string( clearing_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "rows.centers" );
  }

  {
    GnuPlotSettings const settings{
      .title   = format( "Column Based Range Center Densities "
                           "(generated) ({}) [{}]",
                         mode_, maps_ ),
      .x_label = "X (column)",
      .y_label = "Density",
      .x_range = "1:56",
      .y_range = "0:.1",
    };

    CsvData csv_data{
      .header = { "x", "mountains", "hills", "clearing" },
      .rows   = {},
    };

    for( int x = 0; x < map_sz_.w; ++x ) {
      vector<string> row{
        /*x=*/to_string( x + 1 ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
      };
      double const mountains_value =
          lookup( count_range_centers_by_col_[mountains], x )
              .value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      double const hills_value =
          lookup( count_range_centers_by_col_[hills], x )
              .value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      double const clearing_value =
          lookup( count_range_centers_by_col_[clearing], x )
              .value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      row[1] = to_string( mountains_value );
      row[2] = to_string( hills_value );
      row[3] = to_string( clearing_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "cols.centers" );
  }

  {
    GnuPlotSettings const settings{
      .title = format(
          "Row Based Large Densities (generated) ({}) [{}]",
          mode_, maps_ ),
      .x_label = "Y (row)",
      .y_label = "Density",
      .x_range = "1:70",
      .y_range = "0:.005",
    };

    CsvData csv_data{
      .header = { "y", "mountains/10", "hills", "clearing/20" },
      .rows   = {},
    };

    for( int y = 1; y < map_sz_.h - 1; ++y ) {
      vector<string> row{
        /*y=*/to_string( y + 1 ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
      };
      double const mountains_value =
          lookup( count_large_range_by_row_[mountains], y )
              .value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) ) / 10;
      double const hills_value =
          lookup( count_large_range_by_row_[hills], y )
              .value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) );
      double const clearing_value =
          lookup( count_large_range_by_row_[clearing], y )
              .value_or( 0 ) /
          double( lookup( land_by_row_, y ).value_or( 0 ) ) / 20;
      row[1] = to_string( mountains_value );
      row[2] = to_string( hills_value );
      row[3] = to_string( clearing_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "rows.large" );
  }

  {
    GnuPlotSettings const settings{
      .title = format(
          "Column Based Large Densities (generated) ({}) [{}]",
          mode_, maps_ ),
      .x_label = "X (column)",
      .y_label = "Density",
      .x_range = "1:56",
      .y_range = "0:.005",
    };

    CsvData csv_data{
      .header = { "x", "mountains/10", "hills", "clearing/20" },
      .rows   = {},
    };

    for( int x = 0; x < map_sz_.w; ++x ) {
      vector<string> row{
        /*x=*/to_string( x + 1 ),
        /*mountains=*/"",
        /*hills=*/"",
        /*clearing=*/"",
      };
      double const mountains_value =
          lookup( count_large_range_by_col_[mountains], x )
              .value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) ) / 10;
      double const hills_value =
          lookup( count_large_range_by_col_[hills], x )
              .value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) );
      double const clearing_value =
          lookup( count_large_range_by_col_[clearing], x )
              .value_or( 0 ) /
          double( lookup( land_by_col_, x ).value_or( 0 ) ) / 20;
      row[1] = to_string( mountains_value );
      row[2] = to_string( hills_value );
      row[3] = to_string( clearing_value );
      csv_data.rows.push_back( std::move( row ) );
    }

    gnuplot( settings, csv_data, "cols.large" );
  }

  for( auto const type : enum_values<e_terrain_formation> ) {
    string const type_name =
        base::capitalize_initials( base::to_str( type ) );
    double const top_count = [&] {
      switch( type ) {
        case e_terrain_formation::clearing:
          return 1.5;
        case e_terrain_formation::hills:
          return 1.5;
        case e_terrain_formation::mountains:
          return 1.5;
      }
    }();
    double const top_density = [&] {
      switch( type ) {
        case e_terrain_formation::clearing:
          return .25;
        case e_terrain_formation::hills:
          return .25;
        case e_terrain_formation::mountains:
          return .25;
      }
    }();
    {
      GnuPlotSettings const settings{
        .title =
            format( "{} on Biome by Row (generated) ({}) [{}]",
                    type_name, mode_, maps_ ),
        .x_label = "Y (row)",
        .y_label = "Count per Row",
        .x_range = "1:70",
        .y_range = format( "0:{}", top_count ),
      };

      CsvData csv_data{
        .header = { "y" },
        .rows   = {},
      };
      for( e_biome const biome : BIOME_ORDERING )
        csv_data.header.push_back( base::to_str( biome ) );

      for( int y = 0; y < map_sz_.h; ++y ) {
        vector<string> row{ /*y=*/to_string( y + 1 ) };
        auto const biomes =
            lookup( count_with_biome_by_row_[type], y );
        if( !biomes.has_value() ) {
          for( e_biome const _ : BIOME_ORDERING )
            row.push_back( to_string( 0 ) );
          csv_data.rows.push_back( std::move( row ) );
          continue;
        }
        for( e_biome const biome : BIOME_ORDERING )
          row.push_back(
              to_string( ( *biomes )[biome] / maps ) );
        csv_data.rows.push_back( std::move( row ) );
      }

      gnuplot( settings, csv_data,
               format( "biome.counts.rows.{}", type ) );
    }
    {
      GnuPlotSettings const settings{
        .title = format(
            "{} on Biome by Column (generated) ({}) [{}]",
            type_name, mode_, maps_ ),
        .x_label = "X (column)",
        .y_label = "Count per Column",
        .x_range = "1:56",
        .y_range = format( "0:{}", top_count ),
      };

      CsvData csv_data{
        .header = { "x" },
        .rows   = {},
      };
      for( e_biome const biome : BIOME_ORDERING )
        csv_data.header.push_back( base::to_str( biome ) );

      for( int x = 0; x < map_sz_.w; ++x ) {
        vector<string> row{ /*x=*/to_string( x + 1 ) };
        auto const biomes =
            lookup( count_with_biome_by_col_[type], x );
        if( !biomes.has_value() ) {
          for( e_biome const _ : BIOME_ORDERING )
            row.push_back( to_string( 0 ) );
          csv_data.rows.push_back( std::move( row ) );
          continue;
        }
        for( e_biome const biome : BIOME_ORDERING )
          row.push_back(
              to_string( ( *biomes )[biome] / maps ) );
        csv_data.rows.push_back( std::move( row ) );
      }

      gnuplot( settings, csv_data,
               format( "biome.counts.cols.{}", type ) );
    }
    {
      GnuPlotSettings const settings{
        .title = format(
            "{} Density on Biome by Row (generated) ({}) [{}]",
            type_name, mode_, maps_ ),
        .x_label = "Y (row)",
        .y_label = "Density",
        .x_range = "1:70",
        .y_range = format( "0:{}", top_density ),
      };

      CsvData csv_data{
        .header = { "y" },
        .rows   = {},
      };
      for( e_biome const biome : BIOME_ORDERING )
        csv_data.header.push_back( base::to_str( biome ) );

      for( int y = 0; y < map_sz_.h; ++y ) {
        vector<string> row{ /*y=*/to_string( y + 1 ) };
        SCOPE_EXIT {
          csv_data.rows.push_back( std::move( row ) );
        };
        auto const land = lookup( land_with_biome_by_row_, y );
        auto const biomes =
            lookup( count_with_biome_by_row_[type], y );
        if( !land.has_value() ) goto skip;
        if( !biomes.has_value() ) goto skip;
        for( e_biome const biome : BIOME_ORDERING ) {
          if( ( *land )[biome] == 0 ) {
            row.push_back( to_string( 0 ) );
            continue;
          }
          double const val =
              ( *biomes )[biome] / double( ( *land )[biome] );
          row.push_back( to_string( val ) );
        }
        continue;
      skip:
        for( e_biome const _ : BIOME_ORDERING )
          row.push_back( to_string( 0 ) );
      }

      gnuplot( settings, csv_data,
               format( "biome.density.rows.{}", type ) );
    }
    {
      GnuPlotSettings const settings{
        .title   = format( "{} Density on Biome by Column "
                             "(generated) ({}) [{}]",
                           type_name, mode_, maps_ ),
        .x_label = "X (column)",
        .y_label = "Density",
        .x_range = "1:56",
        .y_range = format( "0:{}", top_density ),
      };

      CsvData csv_data{
        .header = { "x" },
        .rows   = {},
      };
      for( e_biome const biome : BIOME_ORDERING )
        csv_data.header.push_back( base::to_str( biome ) );

      for( int x = 0; x < map_sz_.w; ++x ) {
        vector<string> row{ /*y=*/to_string( x + 1 ) };
        SCOPE_EXIT {
          csv_data.rows.push_back( std::move( row ) );
        };
        auto const land = lookup( land_with_biome_by_col_, x );
        auto const biomes =
            lookup( count_with_biome_by_col_[type], x );
        if( !land.has_value() ) goto skip2;
        if( !biomes.has_value() ) goto skip2;
        for( e_biome const biome : BIOME_ORDERING ) {
          if( ( *land )[biome] == 0 ) {
            row.push_back( to_string( 0 ) );
            continue;
          }
          double const val =
              ( *biomes )[biome] / double( ( *land )[biome] );
          row.push_back( to_string( val ) );
        }
        continue;
      skip2:
        for( e_biome const _ : BIOME_ORDERING )
          row.push_back( to_string( 0 ) );
      }

      gnuplot( settings, csv_data,
               format( "biome.density.cols.{}", type ) );
    }
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<IMapStatsCollector>
create_biome_density_stats_collector( std::string const& stem ) {
  return make_unique<BiomeDensityStatsCollector>( stem );
}

unique_ptr<IMapStatsCollector> create_wetness_stats_collector(
    string const& stem, WeatherValue const& climate ) {
  return make_unique<WetnessStatsCollector>( stem, climate );
}

unique_ptr<IMapStatsCollector>
create_biome_wetness_stats_collector(
    WeatherValue const& climate ) {
  return make_unique<BiomeWetnessStatsCollector>( climate );
}

unique_ptr<IMapStatsCollector> create_formations_stats_collector(
    string const& mode ) {
  return make_unique<FormationsStatsCollector>( mode );
}

} // namespace rn
