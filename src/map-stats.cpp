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
  // enum_map<e_biome, double> const& kTargets =
  //     config_map_gen.terrain_generation.biomes.wet_dry_modulation
  //         .for_biome;
  enum_map<e_biome, double> const& kTargets{
    // clang-format off
    // bbmm
    {e_biome::tundra,    0.097},
    {e_biome::desert,    0.187},
    {e_biome::savannah,  0.209},
    {e_biome::plains,    0.283},
    {e_biome::prairie,   0.442},
    {e_biome::grassland, 0.477},
    {e_biome::swamp,     0.580},
    {e_biome::marsh,     0.614},
    // mmmm
    // {e_biome::tundra,    0.140},
    // {e_biome::desert,    0.267},
    // {e_biome::savannah,  0.307},
    // {e_biome::plains,    0.364},
    // {e_biome::prairie,   0.546},
    // {e_biome::grassland, 0.602},
    // {e_biome::swamp,     0.689},
    // {e_biome::marsh,     0.715},
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

} // namespace rn
