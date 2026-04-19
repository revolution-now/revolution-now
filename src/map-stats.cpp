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
#include "terrain-mgr.hpp"

// config
#include "config/map-gen-types.rds.hpp"
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
using ::base::str_replace_all;
using ::gfx::point;
using ::gfx::size;
using ::refl::enum_count;
using ::refl::enum_map;

/****************************************************************
** Global constants.
*****************************************************************/
static array<e_biome, 9> constexpr kGroundTypes = {
  e_biome::savannah, e_biome::grassland, e_biome::tundra,
  e_biome::plains,   e_biome::prairie,   e_biome::desert,
  e_biome::swamp,    e_biome::marsh,     e_biome::arctic,
};
static_assert( kGroundTypes.size() == enum_count<e_biome> );

} // namespace

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
  fs::path const generated =
      "tools/auto-measure/auto-map-gen/biomes/generated";
  ofstream csv( generated / format( "{}.csv", stem_ ) );
  ofstream gnu( generated / format( "{}.gnuplot", stem_ ) );
  CHECK( csv.good() );
  CHECK( gnu.good() );
  // TODO: replace this with new gnuplot module.
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
        value =
            double( *biome_count_y_row_gt ) / *land_count_y_row;
      }
    skip:
      csv << ',' << value;
    }
    csv << '\n';
  }
}

unique_ptr<IMapStatsCollector>
create_biome_density_stats_collector( std::string const& stem ) {
  return make_unique<BiomeDensityStatsCollector>( stem );
}

/****************************************************************
** BiomeAdjacencyStatsCollector
*****************************************************************/
template<typename T>
struct BiomeWetStats {
  T with_swamp_adjacent          = {};
  T with_marsh_adjacent          = {};
  T with_swamp_or_marsh_adjacent = {};
  T with_river_adjacent          = {};
  T with_ocean_adjacent          = {};
  T with_river_or_ocean_adjacent = {};
  T with_any_adjacent            = {};
  T with_river                   = {};
};

struct BiomeAdjacencyStatsCollector : IMapStatsCollector {
  BiomeAdjacencyStatsCollector(
      BiomeClustering const& clustering );

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

 private:
  size map_sz_ = {};
  refl::enum_map<e_biome, BiomeWetStats<double>> wet_;
  refl::enum_map<e_biome, int> adjacency_;
  refl::enum_map<e_biome, int> terrain_total_;
  refl::enum_map<e_biome, std::map<int /*y*/, int /*count*/>>
      ground_per_row_;
  std::map<int /*y*/, int /*count*/> land_per_row_;
  // This gives the total number of surrounding land squares (of
  // any biome) around tiles of the given biome on the given row.
  refl::enum_map<e_biome, std::map<int /*y*/, int /*count*/>>
      surrounding_per_row_;
  int land_total_ = 0;
  int maps_total_ = 0;

 public: // Summary.
  BiomeClustering const clustering_;
  refl::enum_map<e_biome, double> adjacency_baselines;
  refl::enum_map<e_biome, double> relative_adjacencies;
  refl::enum_map<e_biome, BiomeWetStats<double>> wet_results;
  refl::enum_map<e_biome, BiomeWetStats<double>> wet_ratios;
};

BiomeAdjacencyStatsCollector::BiomeAdjacencyStatsCollector(
    BiomeClustering const& clustering )
  : clustering_( clustering ) {}

void BiomeAdjacencyStatsCollector::collect(
    MapMatrix const& m ) {
  map_sz_ = m.size();
  ++maps_total_;
  on_all_tiles( m, [&]( point const tile,
                        MapSquare const& center ) {
    if( center.surface == e_surface::water ) return;
    ++land_total_;
    ++terrain_total_[center.ground];
    ++land_per_row_[tile.y];
    ++ground_per_row_[center.ground][tile.y];
    if( center.river.has_value() )
      ++wet_[center.ground].with_river;

    BiomeWetStats<bool> wet;
    on_surrounding(
        m, tile, [&]( point const, MapSquare const& adjacent ) {
          if( adjacent.surface == e_surface::water ) {
            wet.with_ocean_adjacent = true;
            return;
          }
          // Adjacent square is land.
          if( adjacent.ground == e_biome::swamp )
            wet.with_swamp_adjacent = true;
          if( adjacent.ground == e_biome::marsh )
            wet.with_marsh_adjacent = true;
          if( adjacent.river.has_value() )
            wet.with_river_adjacent = true;
          ++surrounding_per_row_[center.ground][tile.y];
          if( adjacent.ground == center.ground )
            ++adjacency_[adjacent.ground];
        } );

    wet.with_swamp_or_marsh_adjacent =
        wet.with_swamp_adjacent || wet.with_marsh_adjacent;
    wet.with_river_or_ocean_adjacent =
        wet.with_river_adjacent || wet.with_ocean_adjacent;
    wet.with_any_adjacent = wet.with_swamp_or_marsh_adjacent ||
                            wet.with_river_or_ocean_adjacent;

    wet_[center.ground].with_swamp_adjacent +=
        wet.with_swamp_adjacent ? 1.0 : 0.0;
    wet_[center.ground].with_marsh_adjacent +=
        wet.with_marsh_adjacent ? 1.0 : 0.0;
    wet_[center.ground].with_swamp_or_marsh_adjacent +=
        wet.with_swamp_or_marsh_adjacent ? 1.0 : 0.0;
    wet_[center.ground].with_river_adjacent +=
        wet.with_river_adjacent ? 1.0 : 0.0;
    wet_[center.ground].with_ocean_adjacent +=
        wet.with_ocean_adjacent ? 1.0 : 0.0;
    wet_[center.ground].with_river_or_ocean_adjacent +=
        wet.with_river_or_ocean_adjacent ? 1.0 : 0.0;
    wet_[center.ground].with_any_adjacent +=
        wet.with_any_adjacent ? 1.0 : 0.0;
  } );
}

void BiomeAdjacencyStatsCollector::summarize() {
  using enum e_biome;

  for( e_biome const gt : kGroundTypes ) {
    if( gt == arctic ) continue;
    adjacency_baselines[gt] = 0;
    for( auto const& [y, count] : ground_per_row_[gt] ) {
      UNWRAP_CONTINUE( int const land_on_row,
                       lookup( land_per_row_, y ) );
      UNWRAP_CONTINUE( int const surrounding_on_row,
                       lookup( surrounding_per_row_[gt], y ) );
      double const density_at_row =
          double( count ) / land_on_row;
      double const adjacency_baseline_at_row =
          density_at_row * surrounding_on_row;
      adjacency_baselines[gt] += adjacency_baseline_at_row;
    }
    relative_adjacencies[gt] =
        adjacency_[gt] / adjacency_baselines[gt];
  }

  for( e_biome const gt : kGroundTypes ) {
    auto const for_water = clustering_.affinities[gt].for_water;
    if( !for_water.has_value() ) continue;
    double const count = terrain_total_[gt];
    wet_results[gt].with_swamp_adjacent =
        wet_[gt].with_swamp_adjacent / count;
    wet_results[gt].with_ocean_adjacent =
        wet_[gt].with_ocean_adjacent / count;
    wet_ratios[gt].with_ocean_adjacent =
        wet_results[gt].with_ocean_adjacent / *for_water;
  }
}

void BiomeAdjacencyStatsCollector::write() const {
  using enum e_biome;
  double const general_tolerance =
      config_map_gen.terrain_generation.biomes.clustering
          .tolerances.general_adjacency_tolerance;
  double const ocean_tolerance =
      config_map_gen.terrain_generation.biomes.clustering
          .tolerances.ocean_adjacency_tolerance;
  fmt::println( "maps_total: {}", maps_total_ );
  fmt::println( "land_total: {}", land_total_ );
  fmt::println( "tolerance general: {}", general_tolerance );
  fmt::println( "tolerance ocean:   {}", ocean_tolerance );
  fmt::println( "" );

  for( e_biome const gt : kGroundTypes ) {
    double const adjacency_avg =
        double( adjacency_[gt] ) / terrain_total_[gt];
    fmt::println( "{: >10}: {:.3f} | {}/{:.3f} ==> {:.3f}", gt,
                  adjacency_avg, adjacency_[gt],
                  adjacency_baselines[gt],
                  relative_adjacencies[gt] );
  }

  auto const ratio_fmt = [&]( double const ratio,
                              double const tolerance ) {
    auto const color = abs( 1.0 - ratio ) < tolerance
                           ? base::ansi::green
                           : base::ansi::red;
    return format( "{}{:.3f}{}", color, ratio,
                   base::ansi::reset );
  };

  fmt::println( "-----" );

  for( e_biome const gt : kGroundTypes ) {
    double const target =
        pow( 2.0, clustering_.affinities[gt].for_self );
    if( gt == arctic ) continue;
    auto const g = relative_adjacencies[gt];
    fmt::println( "{: >10}: target={}, actual={:.3f}, ratio={}",
                  gt, target, g,
                  ratio_fmt( g / target, general_tolerance ) );
  }

  fmt::println( "-----" );
  for( e_biome const gt : { swamp, marsh } ) {
    fmt::println( "{}:", gt );
    fmt::println(
        "  with_swamp_adjacent:          {:.3f} | ratio={}",
        wet_results[gt].with_swamp_adjacent,
        ratio_fmt( wet_ratios[gt].with_swamp_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_marsh_adjacent:          {:.3f} | ratio={}",
        wet_results[gt].with_marsh_adjacent,
        ratio_fmt( wet_ratios[gt].with_marsh_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_swamp_or_marsh_adjacent: {:.3f} | ratio={}",
        wet_results[gt].with_swamp_or_marsh_adjacent,
        ratio_fmt( wet_ratios[gt].with_swamp_or_marsh_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_river_adjacent:          {:.3f} | ratio={}",
        wet_results[gt].with_river_adjacent,
        ratio_fmt( wet_ratios[gt].with_river_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_ocean_adjacent:          {:.3f} | ratio={}",
        wet_results[gt].with_ocean_adjacent,
        ratio_fmt( wet_ratios[gt].with_ocean_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_river_or_ocean_adjacent: {:.3f} | ratio={}",
        wet_results[gt].with_river_or_ocean_adjacent,
        ratio_fmt( wet_ratios[gt].with_river_or_ocean_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_any_adjacent:            {:.3f} | ratio={}",
        wet_results[gt].with_any_adjacent,
        ratio_fmt( wet_ratios[gt].with_any_adjacent,
                   ocean_tolerance ) );
    fmt::println(
        "  with_river:                   {:.3f} | ratio={}",
        wet_results[gt].with_river,
        ratio_fmt( wet_ratios[gt].with_river,
                   ocean_tolerance ) );
  }
  fmt::println( "-----" );
}

unique_ptr<IMapStatsCollector>
create_biome_adjacency_stats_collector(
    BiomeClustering const& clustering ) {
  return make_unique<BiomeAdjacencyStatsCollector>( clustering );
}

} // namespace rn
