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

// base
#include "base/ansi.hpp"
#include "base/keyval.hpp"
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
** BiomeWetnessStatsCollector
*****************************************************************/
struct BiomeWetnessStatsCollector : IMapStatsCollector {
  BiomeWetnessStatsCollector() = default;

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
                     config_map_gen.terrain_generation.biomes
                         .wet_dry_modulation,
                     res );
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

 private:
  string const mode_;
  size map_sz_    = {};
  int land_       = 0;
  int maps_total_ = 0;

  int land_ocean_adjacent_          = 0;
  int land_ocean_adjacent_cardinal_ = 0;
  int land_non_mounds_              = 0;

  template<typename T>
  using M = enum_map<e_terrain_formation, T>;

  map<int /*row*/, int> land_by_row_;
  map<int /*col*/, int> land_by_col_;
  enum_map<e_biome, int> land_with_biome_;
  map<int /*y*/, enum_map<e_biome, int>> land_with_biome_by_row_;
  M<int> count_;
  int count_forest_ = 0;
  M<int> count_squared_;
  int count_rivers_on_land_ = 0;
  enum_map<e_land_overlay, int> count_arctic_rows_;
  M<enum_map<e_biome, int>> count_with_biome_;
  M<map<int /*y*/, enum_map<e_biome, int>>>
      count_with_biome_by_row_;
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
  ++maps_total_;

  using enum e_terrain_formation;

  on_all_tiles( m, [&]( point const tile,
                        MapSquare const& center ) {
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
      if( has_mountains( center ) )
        ++count_arctic_rows_[e_land_overlay::mountains];
      if( has_hills( center ) )
        ++count_arctic_rows_[e_land_overlay::hills];
      if( has_forest( center ) )
        ++count_arctic_rows_[e_land_overlay::forest];
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
      ++count_by_row_[clearing][tile.y];
      ++count_by_col_[clearing][tile.x];
      if( has_ocean_adjacent ) ++count_ocean_adjacent_[clearing];
      if( has_ocean_adjacent_cardinal )
        ++count_ocean_adjacent_cardinal_[clearing];
      per_map_.tile_to_segment[clearing][tile] = 0;
      ++count_with_biome_[clearing][center.ground];
      ++count_with_biome_by_row_[clearing][tile.y]
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

void FormationsStatsCollector::write() const {
  // TODO
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<IMapStatsCollector>
create_biome_density_stats_collector( std::string const& stem ) {
  return make_unique<BiomeDensityStatsCollector>( stem );
}

unique_ptr<IMapStatsCollector>
create_biome_wetness_stats_collector() {
  return make_unique<BiomeWetnessStatsCollector>();
}

unique_ptr<IMapStatsCollector> create_formations_stats_collector(
    string const& mode ) {
  return make_unique<FormationsStatsCollector>( mode );
}

} // namespace rn
