/****************************************************************
**rivers.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-14.
*
* Description: Handles river generation.
*
*****************************************************************/
#include "rivers.hpp"

// Revolution Now
#include "irand.hpp"
#include "terrain-mgr.hpp"

// config
#include "config/map-gen.hpp"

// ss
#include "ss/terrain.hpp"

// gfx
#include "gfx/iter.hpp"

// base
#include "base/range-lite.hpp"
#include "base/timer.hpp"

// C++ standard library
#include <cmath>
#include <ranges>

namespace rg = std::ranges;
namespace rl = base::rl;

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::ScopedTimer;
using ::gfx::e_cardinal_direction;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::gfx::size;
using ::refl::enum_values;
using ::std::max;
using ::std::min;

/****************************************************************
** Helpers.
*****************************************************************/
// Decays a probability the closer the y coordinate is to either
// the top or bottom map edges.
[[nodiscard]] double river_probability_decay(
    double const edge_decay, int const map_height,
    int const y ) {
  double const half_height = map_height / 2.0;
  double const tiles_from_equator =
      abs( double( y ) - half_height );
  double const tiles_from_equator_normalized =
      tiles_from_equator / half_height;
  double const tiles_from_edge_normalized =
      1.0 - tiles_from_equator_normalized;
  double const decay =
      pow( tiles_from_edge_normalized, edge_decay );
  return clamp( decay, 0.0, 1.0 );
}

} // namespace

/****************************************************************
** RiverParameterInterpolator
*****************************************************************/
double RiverParameterInterpolator::land_form_interp_impl(
    LandFormGetter const fn ) const {
  auto const& map_conf = config_map_gen.terrain_generation;
  auto const& river_conf =
      config_map_gen.terrain_generation.rivers;
  double const field_archipelago =
      fn( river_conf.for_land_form[e_land_form::archipelago] );
  double const field_continents =
      fn( river_conf.for_land_form[e_land_form::continents] );
  double const land_form_measure_range_half_range =
      abs( map_conf.land_layout.land_form
               .customized[e_land_form::continents]
               .scale.mean -
           map_conf.land_layout.land_form
               .customized[e_land_form::archipelago]
               .scale.mean ) /
      2.0;
  double const land_form_measure_range_center =
      map_conf.land_layout.land_form
          .customized[e_land_form::normal]
          .scale.mean;
  double const land_form_measure =
      clamp( ( scale_ - land_form_measure_range_center ) /
                 land_form_measure_range_half_range,
             -1.0, 1.0 );
  double const interp = land_form_measure / 2.0 + 0.5;
  return lerp( field_archipelago, field_continents, interp );
}

double RiverParameterInterpolator::climate_interp_impl(
    ClimateGetter const fn ) const {
  auto const& river_conf =
      config_map_gen.terrain_generation.rivers;
  double const field_arid =
      fn( river_conf.for_climate[e_climate::arid] );
  double const field_normal =
      fn( river_conf.for_climate[e_climate::normal] );
  double const field_wet =
      fn( river_conf.for_climate[e_climate::wet] );
  if( climate_ < 0 )
    return lerp( field_normal, field_arid, climate_ / 100.0 );
  else if( climate_ > 0 )
    return lerp( field_normal, field_wet, climate_ / 100.0 );
  else
    return field_normal;
}

/****************************************************************
** Public API.
*****************************************************************/
struct RiverTile {
  point tile;
  e_river type = {};

  auto operator<=>( RiverTile const& ) const = default;
};

using RiverTiles = vector<RiverTile>;

// NOTE: This function is given prev_river_tile because we can't
// necessarily infer it from `river_tiles` in the case of forks.
static void add_river_land_components(
    MapMatrix const& m, IRand& rand,
    RiverParameters const& params, point const start,
    e_cardinal_direction const from, RiverTiles& river_tiles,
    RiverTile const prev_river_tile ) {
  auto const has_tile_in_history =
      []( RiverTiles const& river_tiles, point const p ) {
        for( RiverTile const& rt : river_tiles )
          if( rt.tile == p ) //
            return true;
        return false;
      };

  CHECK( !river_tiles.empty() );
  CHECK( !m[start].river.has_value() );
  bool const should_grow =
      ( ssize( river_tiles ) < params.min_length ) ||
      rand.bernoulli( params.growth.probability *
                      river_probability_decay( params.edge_decay,
                                               m.size().h,
                                               start.y ) );
  if( !should_grow ) return;

  e_river const type = [&] {
    if( river_tiles.size() == 1 ) {
      return rand.bernoulli( params.sustain_major.probability )
                 ? e_river::major
                 : e_river::minor;
    }
    e_river const prev_type = prev_river_tile.type;
    if( prev_type == e_river::minor )
      return e_river::minor;
    else // major
      return rand.bernoulli( params.sustain_major.probability )
                 ? e_river::major
                 : e_river::minor;
  }();

  CHECK( !m[start].river.has_value(),
         "tile {} already has a river.", start );
  river_tiles.push_back(
      RiverTile{ .tile = start, .type = type } );

  e_cardinal_direction const forward          = from;
  array<e_cardinal_direction, 2> const turned = [&] {
    array<e_cardinal_direction, 2> res;
    res[0] = rotated_cw( from );
    res[1] = rotated_ccw( from );
    if( rand.bernoulli( .5 ) ) swap( res[0], res[1] );
    return res;
  }();

  vector<e_cardinal_direction> const direction_candidates = [&] {
    vector<e_cardinal_direction> res;
    res.reserve( 3 );
    bool const try_turn =
        rand.bernoulli( params.turn.probability );
    if( try_turn ) {
      res.push_back( turned[0] );
      res.push_back( turned[1] );
      res.push_back( forward );
    } else {
      res.push_back( forward );
      res.push_back( turned[0] );
      res.push_back( turned[1] );
    }
    return res;
  }();

  vector<e_cardinal_direction> const available_directions = [&] {
    vector<e_cardinal_direction> res;
    res.reserve( 3 );
    for( e_cardinal_direction const d : direction_candidates ) {
      point const moved       = start.moved( d );
      MapSquare const& square = m[moved];
      if( moved.y == 0 || moved.y == m.size().h - 1 ) continue;
      if( square.river.has_value() ) continue;
      if( square.surface == e_surface::water ) continue;
      if( has_tile_in_history( river_tiles, moved ) ) continue;
      bool const has_surrounding_river = [&] {
        bool res = false;
        on_surrounding_cardinal(
            m, moved,
            [&]( point const, MapSquare const& square,
                 e_cardinal_direction const ) {
              if( square.river.has_value() ) res = true;
            } );
        return res;
      }();
      if( has_surrounding_river ) continue;
      res.push_back( d );
    }
    return res;
  }();

  vector<e_cardinal_direction> const ranked_directions = [&] {
    vector<e_cardinal_direction> res = available_directions;
    auto const close_to_previous =
        [&]( e_cardinal_direction const d ) {
          point const moved         = start.moved( d );
          bool adjacent_to_previous = false;
          on_surrounding_cardinal(
              m, moved,
              [&]( point const p, MapSquare const&,
                   e_cardinal_direction const d2 ) {
                if( d2 == reverse_direction( d ) ) return;
                adjacent_to_previous =
                    adjacent_to_previous ||
                    has_tile_in_history( river_tiles, p );
              } );
          return adjacent_to_previous;
        };
    auto const less = [&]( e_cardinal_direction const l,
                           e_cardinal_direction const r ) {
      bool const l_close_to_previous = close_to_previous( l );
      bool const r_close_to_previous = close_to_previous( r );
      if( l_close_to_previous != r_close_to_previous )
        return l_close_to_previous < r_close_to_previous;
      return false;
    };
    rg::stable_sort( res, less );
    return res;
  }();

  for( e_cardinal_direction const d : ranked_directions ) {
    RiverTiles new_tiles          = river_tiles;
    RiverTile const pre_fork_back = new_tiles.back();
    add_river_land_components( m, rand, params, start.moved( d ),
                               d, new_tiles, pre_fork_back );
    bool const did_main_leg =
        new_tiles.size() > river_tiles.size();
    bool const did_fork = [&] {
      if( !did_main_leg ) return false;
      auto const fork_direction =
          [&] -> maybe<e_cardinal_direction> {
        vector<e_cardinal_direction> available_fork_directions;
        available_fork_directions.reserve( 4 );
        for( e_cardinal_direction const d_fork :
             enum_values<e_cardinal_direction> ) {
          if( d_fork == d ) continue;
          if( rg::find( ranked_directions, d_fork ) ==
              ranked_directions.end() )
            continue;
          // Test if the first leg that we just did above has al-
          // ready placed a river on this spot.
          if( has_tile_in_history( new_tiles,
                                   start.moved( d_fork ) ) )
            continue;
          available_fork_directions.push_back( d_fork );
        }
        return rand.pick_one_safe( available_fork_directions );
      }();
      CHECK( fork_direction != d );
      bool const should_fork =
          fork_direction.has_value() &&
          rand.bernoulli( params.fork.probability );
      if( !should_fork ) return false;
      CHECK( fork_direction.has_value() );
      int const pre_fork_size = new_tiles.size();
      add_river_land_components(
          m, rand, params, start.moved( *fork_direction ),
          *fork_direction, new_tiles, pre_fork_back );
      int const post_fork_size = new_tiles.size();
      return post_fork_size > pre_fork_size;
    }();
    if( ssize( new_tiles ) >= params.min_length ) {
      int const last_idx = river_tiles.size() - 1;
      river_tiles        = new_tiles;
      // If we ended up forking then recompute the fork type.
      if( did_fork )
        river_tiles[last_idx].type =
            rand.bernoulli( params.fork_is_major.probability )
                ? e_river::major
                : e_river::minor;
      CHECK( river_tiles.size() == new_tiles.size() );
      return;
    }
  }
}

#if 0
static maybe<RiverTiles> add_river(
    MapMatrix const& m, IRand& rand,
    RiverParameters const& params, point const start ) {
  auto const initial = RiverTile{
    .tile = start,
    .type = rand.bernoulli( params.start_major_probability )
                ? e_river::major
                : e_river::minor };
  vector<e_cardinal_direction> directions;
  on_surrounding_cardinal(
      m, start,
      [&]( point const p, MapSquare const& square,
           e_cardinal_direction const d ) {
        if( p.y == 0 || p.y == m.size().h - 1 ) return;
        if( square.surface == e_surface::water ) return;
        if( square.river.has_value() ) return;
        bool const has_surrounding_river = [&] {
          bool res = false;
          on_surrounding_cardinal(
              m, p,
              [&]( point const, MapSquare const& square,
                   e_cardinal_direction const ) {
                if( square.river.has_value() ) res = true;
              } );
          return res;
        }();
        if( has_surrounding_river ) return;
        directions.push_back( d );
      } );
  if( directions.empty() ) return nothing;
  rand.shuffle( directions );
  for( e_cardinal_direction const d : directions ) {
    RiverTiles river_tiles;
    river_tiles.push_back( initial );
    point const moved = start.moved( d );
    CHECK( !m[moved].river.has_value() );
    add_river_land_components( m, rand, params, moved, d,
                               river_tiles, initial );
    if( ssize( river_tiles ) >= params.min_length )
      return river_tiles;
  }
  return nothing;
}

void add_rivers( MapMatrix& m, IRand& rand,
                 RiverParameters const& params ) {
  ScopedTimer const timer( "river generation time" );
  set<point> const starting_tiles = [&] {
    set<point> res;
    on_all_tiles(
        m, [&]( point const p, MapSquare const& center ) {
          // Don't count arctic land tiles.
          if( p.y == 0 || p.y == m.size().h - 1 ) return;
          if( center.surface == e_surface::land ) return;
          on_surrounding_cardinal(
              m, p,
              [&]( point const p2, MapSquare const& adjacent,
                   e_cardinal_direction ) {
                // Don't count arctic land tiles.
                if( p2.y == 0 || p2.y == m.size().h - 1 ) return;
                if( adjacent.surface == e_surface::water )
                  return;
                res.insert( p );
              } );
        } );
    return res;
  }();
  vector<point> chosen;
  chosen.reserve( starting_tiles.size() );
  for( point const p : starting_tiles ) {
    double const row_probability =
        params.initiation_probability *
        river_probability_decay( params.edge_decay, m.size().h, p.y );
    if( rand.bernoulli( row_probability ) )
      chosen.push_back( p );
  }
  rand.shuffle( chosen );

  for( point const start : chosen ) {
    CHECK( start.y != 0 && start.y != m.size().h - 1 );
    auto const river_tiles = add_river( m, rand, params, start );
    if( !river_tiles.has_value() ) continue;
    // Note that consecutive river tiles in this vector are not
    // necessarily adjacent to each other due to forking.
    for( auto const [i, rt] :
         rl::all( *river_tiles ).enumerate() ) {
      CHECK( rt.tile.is_inside( m.rect() ) );
      if( i == 0 )
        CHECK( m[rt.tile].surface == e_surface::water );
      else
        CHECK( m[rt.tile].surface == e_surface::land );
      CHECK( !m[rt.tile].river.has_value(),
             "tile {} already has a river. i={}", rt.tile, i );
      m[rt.tile].river = rt.type;
    }
  }
}

#else

void add_rivers( MapMatrix& m, IRand& rand,
                 RiverParameters const& params ) {
  ScopedTimer const timer( "river generation time" );
  set<point> const starting_tiles = [&] {
    set<point> res;
    on_all_tiles(
        m, [&]( point const p, MapSquare const& center ) {
          // Don't count arctic land tiles.
          if( p.y == 0 || p.y == m.size().h - 1 ) return;
          if( center.surface == e_surface::water ) return;
          on_surrounding_cardinal(
              m, p,
              [&]( point const p2, MapSquare const& adjacent,
                   e_cardinal_direction ) {
                // Don't count arctic land tiles.
                if( p2.y == 0 || p2.y == m.size().h - 1 ) return;
                if( adjacent.surface == e_surface::land ) return;
                res.insert( p );
              } );
        } );
    return res;
  }();
  vector<point> chosen;
  chosen.reserve( starting_tiles.size() );
  for( point const p : starting_tiles ) {
    double const row_probability =
        params.initiation.probability *
        river_probability_decay( params.edge_decay, m.size().h,
                                 p.y );
    if( rand.bernoulli( row_probability ) )
      chosen.push_back( p );
  }
  rand.shuffle( chosen );

  auto const is_open_tile = [&]( point const p ) {
    if( m[p].river.has_value() ) return false;
    bool has_surrounding_river = false;
    on_surrounding_cardinal(
        m, p,
        [&]( point const, MapSquare const& adjacent,
             e_cardinal_direction ) {
          if( adjacent.river.has_value() )
            has_surrounding_river = true;
        } );
    if( has_surrounding_river ) return false;
    return true;
  };

  vector<e_cardinal_direction> waters;
  for( point const start : chosen ) {
    if( !is_open_tile( start ) )
      // A previous iteration on another start tile could have
      // caused a river to be placed here.
      continue;
    waters.clear();
    CHECK( start.y != 0 && start.y != m.size().h - 1 );
    for( e_cardinal_direction const d :
         enum_values<e_cardinal_direction> ) {
      point const moved = start.moved( d );
      if( m[moved].surface != e_surface::water ) continue;
      if( moved.y == 0 || moved.y == m.size().h - 1 ) continue;
      waters.push_back( d );
    }
    if( waters.empty() ) continue;
    e_cardinal_direction const d_water = rand.pick_one( waters );
    point const water                  = start.moved( d_water );
    RiverTiles river_tiles;
    auto const initial = RiverTile{
      .tile = water,
      .type = rand.bernoulli( params.start_major.probability )
                  ? e_river::major
                  : e_river::minor };
    river_tiles.push_back( initial );
    add_river_land_components( m, rand, params, start,
                               reverse_direction( d_water ),
                               river_tiles, initial );
    if( ssize( river_tiles ) < params.min_length ) continue;
    // We have a well-formed river, so lay it down. Note that
    // consecutive river tiles in this vector are not necessarily
    // adjacent to each other due to forking.
    for( auto const [i, rt] :
         rl::all( river_tiles ).enumerate() ) {
      CHECK( rt.tile.is_inside( m.rect() ) );
      if( i == 0 )
        CHECK( m[rt.tile].surface == e_surface::water );
      else
        CHECK( m[rt.tile].surface == e_surface::land );
      CHECK( !m[rt.tile].river.has_value(),
             "tile {} already has a river. i={}", rt.tile, i );
      m[rt.tile].river = rt.type;
    }
  }
}
#endif

int count_rivers( MapMatrix const& m ) {
  int total = 0;
  for( point const p : rect_iterator( m.rect() ) )
    if( m[p].river.has_value() ) ++total;
  return total;
}

} // namespace rn
