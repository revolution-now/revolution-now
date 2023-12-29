/****************************************************************
**connectivity.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-12-20.
*
* Description: Algorithm to populate the map connectivity
*              sections of the SAV file in a way that is intended
*              to reproduce the OG's intended behavior.
*
*****************************************************************/
#include "connectivity.hpp"

// sav
#include "sav-struct.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/timer.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace sav {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::size;

bool quad_exists( point q ) {
  if( q.y < 0 || q.x < 0 ) return false;
  if( q.y >= 18 || q.x >= 15 ) return false;
  return true;
}

bool tile_exists( point q ) {
  if( q.y < 0 || q.x < 0 ) return false;
  if( q.y >= 72 || q.x >= 58 ) return false;
  return true;
}

bool is_tile_water( terrain_5bit_type tile ) {
  return ( tile == terrain_5bit_type::ttt ||
           tile == terrain_5bit_type::tnt );
}

maybe<point> find_anchor_impl( point const q,
                               auto&&      tile_valid ) {
  static auto anchor_deltas = {
      size{ .w = 1, .h = 1 },
      size{ .w = 1, .h = 2 },
      size{ .w = 2, .h = 1 },
      size{ .w = 2, .h = 2 },
  };
  point const p = q * 4;
  for( size const s : anchor_deltas ) {
    point const candidate = p + s;
    if( !tile_exists( candidate ) ) continue;
    int const offset = candidate.y * 58 + candidate.x;
    if( tile_valid( offset ) ) return candidate;
  }
  return nothing;
}

// This is similar to the A* algorithm, but is not guaranteed to
// return the shortest path; it will potentially return any valid
// path whose distances is <= to upper_bound.
bool has_path( point const src, point const dst, int upper_bound,
               auto connected_fn ) {
  unordered_map<point, /*distance=*/int>     explore;
  unordered_map<point, /*min_distance=*/int> explored;
  explore[src] = 0;
  while( !explore.empty() ) {
    // Find the point in the `explore` list that is closest to
    // the destination by pythagorean distance in the hopes that
    // this will be the optimal strategy to get to the destina-
    // tion as quickly as possible (it usually is).
    auto [p, dist] = *[&] {
      double shortest_distance = numeric_limits<double>::max();
      auto   shortest_it       = explore.begin();
      for( auto it = explore.begin(); it != explore.end();
           ++it ) {
        auto [p, _] = *it;
        double const dist =
            sqrt( ( dst.x - p.x ) * ( dst.x - p.x ) +
                  ( dst.y - p.y ) * ( dst.y - p.y ) );
        if( dist < shortest_distance ) {
          shortest_distance = dist;
          shortest_it       = it;
        }
      }
      return shortest_it;
    }();
    explore.erase( p );
    CHECK( !explored.contains( p ) );
    explored[p] = dist;
    if( p == dst && dist <= upper_bound ) return true;
    int const new_dist = dist + 1;
    if( new_dist > upper_bound ) continue;
    for( int dy = -1; dy <= 1; ++dy ) {
      for( int dx = -1; dx <= 1; ++dx ) {
        if( dx == 0 && dy == 0 ) continue;
        size const  d{ .w = dx, .h = dy };
        point const new_p = p + d;
        if( !connected_fn( p, new_p ) ) continue;
        if( explored.contains( new_p ) ) {
          if( explored[new_p] <= new_dist ) continue;
          explored.erase( new_p );
        }
        // explored does not contain new_p at this point.
        if( explore.contains( new_p ) ) {
          if( explore[new_p] <= new_dist ) continue;
          explore.erase( new_p );
        }
        // explore does not contain new_p at this point.
        explore[new_p] = new_dist;
      }
    }
  }
  return false;
}

bool has_6_distance_or_less( vector<TILE> const& tiles,
                             vector<PATH> const& path, point p1,
                             point p2 ) {
  auto connected = [&]( point src, point dst ) {
    CHECK_LE( abs( src.x - dst.x ), 1 );
    CHECK_LE( abs( src.y - dst.y ), 1 );
    if( !tile_exists( src ) || !tile_exists( dst ) )
      return false;
    int const src_offset = src.y * 58 + src.x;
    int const dst_offset = dst.y * 58 + dst.x;
    CHECK_LT( src_offset, int( tiles.size() ) );
    CHECK_LT( dst_offset, int( tiles.size() ) );
    // Tiles not visible in-game don't count. The OG assigns a 0
    // region type to these "border" tiles.
    if( path[dst_offset].region_id == region_id_4bit_type::_0 )
      return false;
    return is_tile_water( tiles[src_offset].tile ) ==
           is_tile_water( tiles[dst_offset].tile );
  };
  return has_path( p1, p2, 6, connected );
}

template<typename T, typename Func>
void populate_connectivity_impl( array<T, 270>& connectivity,
                                 Func&& has_connection ) {
  CHECK_EQ( int( connectivity.size() ), 18 * 15 );
  auto connectivity_quad = [&]( point q ) -> maybe<T&> {
    if( !quad_exists( q ) ) return nothing;
    int const offset = q.x * 18 + q.y;
    CHECK_LT( offset, int( connectivity.size() ) );
    return connectivity[offset];
  };

  for( int qy = 0; qy < 18; ++qy ) {
    for( int qx = 0; qx < 15; ++qx ) {
      int quad_delta_y = 0;
      int quad_delta_x = 0;

      auto connectivity_quads = [&] {
        return pair{
            connectivity_quad( { .x = qx, .y = qy } ),
            connectivity_quad( { .x = qx + quad_delta_x,
                                 .y = qy + quad_delta_y } ),
        };
      };

      auto if_connected_do = [&]( auto action ) {
        auto [fst, snd] = connectivity_quads();
        if( fst.has_value() && snd.has_value() &&
            has_connection( { .x = qx, .y = qy },
                            { .x = qx + quad_delta_x,
                              .y = qy + quad_delta_y } ) )
          action( fst, snd );
      };

      // Because of the way we're iterating over    ? = (qy,qx)
      // the tiles (left to right, then top to     +---+---+---+
      // bottom), and because connections are al-  |   |   |   |
      // ways made in a symmetric way, that means  +---+---+---+
      // that for each new quad that we're com-    |   | ? | 4 |
      // puting, we only need to compute the con-  +---+---+---+
      // nections for the four surrounding quads   | 1 | 2 | 3 |
      // marked with numbers, since the others     +---+---+---+
      // will have already been filled in:

      // #1
      quad_delta_x = -1;
      quad_delta_y = 1;
      if_connected_do( []( auto fst, auto snd ) {
        fst->swest = true;
        snd->neast = true;
      } );

      // #2
      quad_delta_x = 0;
      quad_delta_y = 1;
      if_connected_do( []( auto fst, auto snd ) {
        fst->south = true;
        snd->north = true;
      } );

      // #3
      quad_delta_x = 1;
      quad_delta_y = 1;
      if_connected_do( []( auto fst, auto snd ) {
        fst->seast = true;
        snd->nwest = true;
      } );

      // #4
      quad_delta_x = 1;
      quad_delta_y = 0;
      if_connected_do( []( auto fst, auto snd ) {
        fst->east = true;
        snd->west = true;
      } );
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void populate_connectivity( vector<TILE> const& tiles,
                            vector<PATH> const& path,
                            CONNECTIVITY&       connectivity ) {
  auto has_connection = [&]( point q1, point q2,
                             auto&& find_anchor ) {
    CHECK( quad_exists( q1 ) );
    CHECK( quad_exists( q2 ) );
    maybe<point> const anchor1 = find_anchor( q1 );
    if( !anchor1.has_value() ) return false;
    maybe<point> const anchor2 = find_anchor( q2 );
    if( !anchor2.has_value() ) return false;
    return has_6_distance_or_less( tiles, path, *anchor1,
                                   *anchor2 );
  };

  // Sea lane connectivity.
  {
    base::ScopedTimer timer( "populate_sea_lane_connectivity" );
    populate_connectivity_impl(
        connectivity.sea_lane_connectivity,
        [&]( point p1, point p2 ) {
          return has_connection( p1, p2, [&]( point q ) {
            return find_anchor_impl( q, [&]( int offset ) {
              CHECK_LT( offset, int( path.size() ) );
              // _1 is a value of 1 which the OG uses for all
              // tiles that are connected to either the left or
              // right edge of the map, even if the left and
              // right edges are not themselves connected via wa-
              // ter. Note that a region ID of 1 is also used for
              // one of the land masses, but that will be ruled
              // out when we check for water below.
              if( path[offset].region_id !=
                  region_id_4bit_type::_1 )
                return false;
              return is_tile_water( tiles[offset].tile );
            } );
          } );
        } );
  }

  // Land connectivity.
  {
    base::ScopedTimer timer( "populate_land_connectivity" );
    populate_connectivity_impl(
        connectivity.land_connectivity,
        [&]( point p1, point p2 ) {
          return has_connection( p1, p2, [&]( point q ) {
            return find_anchor_impl( q, [&]( int offset ) {
              return !is_tile_water( tiles[offset].tile );
            } );
          } );
        } );
  }
}

} // namespace sav
