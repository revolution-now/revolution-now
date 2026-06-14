/****************************************************************
**formations.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-05-17.
*
* Description: Generates land overlay formations.
*
*****************************************************************/
#include "formations.hpp"

// Revolution Now
#include "irand.hpp"
#include "map-square.hpp"
#include "terrain-mgr.hpp"

// config
#include "config/map-gen-types.hpp"
#include "config/range-helpers.hpp"

// ss
#include "ss/map-matrix.hpp"

// refl
#include "refl/query-enum.hpp"

// C++ standard library
#include <ranges>

namespace rn {

namespace {

namespace rg = std::ranges;

using namespace std;

using ::base::function_ref;
using ::base::maybe;
using ::base::nothing;
using ::gfx::dsize;
using ::gfx::e_cardinal_cdirection;
using ::gfx::point;
using ::gfx::size;
using ::refl::enum_map;
using ::refl::enum_values;
using ::rn::config::Probability;

void on_spawnable(
    MapMatrix& m,
    function_ref<void( point, MapSquare& square )> fn ) {
  on_all_tiles( m, [&]( point const p, MapSquare& square ) {
    if( square.surface == e_surface::water ) return;
    if( square.ground == e_biome::arctic ) return;
    if( terrain_formation_for( square ).has_value() ) return;
    fn( p, square );
  } );
}

void run_organic_growth(
    IRand& rand, MapMatrix& m,
    e_terrain_formation const formation,
    function_ref<Probability( point ) const> const
        spawn_probability,
    double const growth_factor, int const max_length,
    point const start ) {
  set<point> points_can_grow;
  auto const update_can_grow = [&]( point const p,
                                    bool const can_add ) {
    for( auto const d : enum_values<e_cardinal_direction> ) {
      point const moved = p.moved( d );
      if( !m.exists( moved ) ) continue;
      if( is_water( m[moved] ) ) continue;
      if( m[moved].ground == e_biome::arctic ) continue;
      if( terrain_formation_for( m[moved] ).has_value() )
        continue;
      if( can_add ) points_can_grow.insert( p );
      return;
    }
    points_can_grow.erase( p );
  };
  int placed       = 0;
  auto const place = [&]( point const p ) {
    ++placed;
    CHECK( m[p].surface == e_surface::land );
    CHECK( m[p].ground != e_biome::arctic );
    CHECK( !terrain_formation_for( m[p] ).has_value() );
    assign_formation( m[p], formation );
    update_can_grow( p, true );
    for( auto const d : enum_values<e_cardinal_direction> ) {
      point const moved = p.moved( d );
      if( !m.exists( moved ) ) continue;
      update_can_grow( moved, false );
    }
  };
  vector<e_cardinal_direction> ds( 4 );
  auto const grow_tile = [&]( point const p ) {
    rg::copy( enum_values<e_cardinal_direction>, ds.begin() );
    rand.shuffle( ds );
    for( auto const d : ds ) {
      point const moved = p.moved( d );
      if( !m.exists( moved ) ) continue;
      if( is_water( m[moved] ) ) continue;
      if( terrain_formation_for( m[moved] ).has_value() )
        continue;
      place( moved );
      return;
    }
    FATAL( "failed to grow tile {}", p );
  };
  place( start );
  bool grew = true;
  while( true ) {
    if( !grew ) break;
    if( points_can_grow.empty() ) break;
    if( placed >= max_length ) break;
    vector can_grow( points_can_grow.begin(),
                     points_can_grow.end() );
    rand.shuffle( can_grow );
    grew = false;
    for( point const p : can_grow ) {
      double const growth_probability = std::min(
          spawn_probability( p ).probability * growth_factor,
          1.0 );
      if( !rand.bernoulli( growth_probability ) ) continue;
      grow_tile( p );
      grew = true;
      break;
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void set_all_forest( MapMatrix& m ) {
  on_all_tiles( m, [&]( point const, MapSquare& square ) {
    if( square.surface == e_surface::water ) return;
    if( square.ground == e_biome::arctic ) return;
    square.overlay = e_land_overlay::forest;
  } );
}

void generate_formation(
    IRand& rand, MapMatrix& m,
    e_terrain_formation const formation,
    enum_map<e_biome, Probability> const& biome_density,
    Probability const spawn, double const growth_factor,
    dsize const edge_decay, int const max_length ) {
  size const sz                = m.size();
  double const wd2             = sz.w / 2.0;
  double const hd2             = sz.h / 2.0;
  auto const spawn_probability = [&]( point const p ) {
    double const x_dist  = pow( abs( p.x - wd2 ) / wd2, 4.0 );
    double const y_dist  = pow( abs( p.y - hd2 ) / hd2, 1.0 );
    double const decay_w = pow( 1.0 - edge_decay.w, x_dist );
    double const decay_h = pow( 1.0 - edge_decay.h, y_dist );
    double const decay   = decay_w * decay_h;
    Probability const p_decay{ .probability = decay };
    CHECK( p_decay.validate() );
    return spawn * biome_density[m[p].ground] * p_decay;
  };
  on_spawnable( m, [&]( point const p, MapSquare const& ) {
    if( rand.bernoulli( spawn_probability( p ) ) )
      run_organic_growth( rand, m, formation, spawn_probability,
                          growth_factor, max_length, p );
  } );
}

} // namespace rn
