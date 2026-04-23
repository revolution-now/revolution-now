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
  size map_sz_ = {};
  refl::enum_map<e_biome, double> biome_wetness_;
  refl::enum_map<e_biome, int> biome_count_;
  int maps_total_ = 0;
};

void BiomeWetnessStatsCollector::collect( MapMatrix const& m ) {
  map_sz_ = m.size();
  ++maps_total_;

  matrix<double> const wetness = [&] {
    matrix<double> res;
    compute_wetness( m, res );
    CHECK_EQ( res.size(), m.size() );
    return res;
  }();

  on_all_tiles(
      m, [&]( point const tile, MapSquare const& center ) {
        if( center.surface == e_surface::water ) return;
        ++biome_count_[center.ground];
        biome_wetness_[center.ground] += wetness[tile];
      } );
}

void BiomeWetnessStatsCollector::summarize() {}

void BiomeWetnessStatsCollector::write() const {
  static enum_map<e_biome, double> const kTargets{
    // clang-format off
    {e_biome::tundra,    -0.698},
    {e_biome::desert,    -0.454},
    {e_biome::savannah,  -0.374},
    {e_biome::plains,    -0.205},
    {e_biome::arctic,     0.000},
    {e_biome::prairie,    0.216},
    {e_biome::grassland,  0.316},
    {e_biome::swamp,      0.559},
    {e_biome::marsh,      0.642},
    // clang-format on
  };
  double avg = 0;
  for( e_biome const biome : kWetOrdering ) {
    if( biome == e_biome::arctic ) continue;
    double const wetness =
        biome_wetness_[biome] / biome_count_[biome];
    avg += wetness;
  }
  avg /= 8; // 9-1 since no arctic.
  fmt::println( "{:12}: {:>12} {:>12} {:>12}", "biome",
                "wetness", "normalized", "ratio" );
  fmt::println(
      "-----------------------------------------------------" );
  for( e_biome const biome : kWetOrdering ) {
    if( biome == e_biome::arctic ) continue;
    double const wetness =
        biome_wetness_[biome] / biome_count_[biome];
    double const normalized = ( wetness / avg ) - 1;
    fmt::println( "{:12}: {:12.3f} {:12.3f} {:12.3f}", biome,
                  wetness, normalized,
                  normalized / kTargets[biome] );
  }
  fmt::println( "try next:" );
  enum_map<e_biome, double> next;
  double total = 0;
  for( e_biome const biome : kWetOrdering ) {
    if( biome == e_biome::arctic ) continue;
    double const wetness =
        biome_wetness_[biome] / biome_count_[biome];
    double const normalized = ( wetness / avg ) - 1;
    double const scale      = normalized / kTargets[biome];
    next[biome] = config_map_gen.terrain_generation.biomes
                      .wet_dry_modulation.for_biome[biome] /
                  scale;
    total += next[biome];
  }
  next[e_biome::arctic] = 0.0;
  avg                   = total / 8;
  // for( e_biome const biome : kWetOrdering ) {
  //   if( biome == e_biome::arctic ) continue;
  //   next[biome] -= avg;
  // }
  for( e_biome const biome : kWetOrdering )
    fmt::println( "        {:11}{:>6.3f}  # {:>6.3f}",
                  base::to_str( biome ) + ':', next[biome],
                  kTargets[biome] );
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
