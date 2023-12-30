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
#include "base/cc-specific.hpp"
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

/****************************************************************
** Helpers
*****************************************************************/
bool is_tile_water( terrain_5bit_type tile ) {
  return ( tile == terrain_5bit_type::ttt ||
           tile == terrain_5bit_type::tnt );
}

bool is_tile_land( terrain_5bit_type tile ) {
  return !is_tile_water( tile );
}

// This is similar to the A* algorithm, but it is "weaker" in
// that it does not need to find the shortest path; it just re-
// turns true as soon as it finds a path with length <=
// upper_bound. This is because the consumer doesn't care what
// the actual path is, it just cares that the destination is
// reachable from the source in a certain number of jumps.
bool has_path( point const src, point const dst, int upper_bound,
               auto&& are_tiles_connected ) {
  unordered_map<point, /*distance=*/int>     explore;
  unordered_map<point, /*min_distance=*/int> explored;
  explore[src] = 0;
  while( !explore.empty() ) {
    auto const [p, dist] = *[&] {
      // This block is an optimization and not stricly necessary.
      // It finds the point in the `explore` list that is closest
      // to the destination by pythagorean distance in the hopes
      // that this will be the optimal strategy to get to the
      // destination as quickly as possible (it usually is).
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
        if( !are_tiles_connected( p, new_p ) ) continue;
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

/****************************************************************
** ConnectivityFinder
*****************************************************************/
struct ConnectivityFinder {
  template<typename T, typename Func>
  void populate_connectivity(
      array<T, 270>& connectivity,
      Func&&         is_valid_anchor_tile ) const;

  bool are_quads_connected( point q1, point q2,
                            auto&& is_valid_anchor_tile ) const;

  bool are_tiles_connected( point src, point dst ) const;

  maybe<point> find_anchor( point const q,
                            auto&&      tile_valid ) const;

  bool quad_exists( point q ) const {
    if( q.y < 0 || q.x < 0 ) return false;
    if( q.y >= map_height_quads_ || q.x >= map_width_quads_ )
      return false;
    return true;
  }

  bool tile_exists( point q ) const {
    if( q.y < 0 || q.x < 0 ) return false;
    if( q.y >= map_height_ || q.x >= map_width_ ) return false;
    return true;
  }

  vector<TILE> const& tiles_;
  vector<PATH> const& path_;

  int const map_width_;
  int const map_height_;

  int const map_width_quads_  = ( map_width_ + 3 ) / 4;
  int const map_height_quads_ = ( map_height_ + 3 ) / 4;
};

maybe<point> ConnectivityFinder::find_anchor(
    point const q, auto&& tile_valid ) const {
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
    int const offset = candidate.y * map_width_ + candidate.x;
    if( tile_valid( offset ) ) return candidate;
  }
  return nothing;
}

bool ConnectivityFinder::are_tiles_connected( point src,
                                              point dst ) const {
  CHECK_LE( abs( src.x - dst.x ), 1 );
  CHECK_LE( abs( src.y - dst.y ), 1 );
  if( !tile_exists( src ) || !tile_exists( dst ) ) return false;
  int const src_offset = src.y * map_width_ + src.x;
  int const dst_offset = dst.y * map_width_ + dst.x;
  CHECK_LT( src_offset, int( tiles_.size() ) );
  CHECK_LT( dst_offset, int( tiles_.size() ) );
  CHECK_LT( src_offset, int( path_.size() ) );
  CHECK_LT( dst_offset, int( path_.size() ) );
  return ( path_[dst_offset].region_id ==
           path_[src_offset].region_id ) &&
         ( is_tile_water( tiles_[src_offset].tile ) ==
           is_tile_water( tiles_[dst_offset].tile ) );
}

bool ConnectivityFinder::are_quads_connected(
    point q1, point q2, auto&& is_valid_anchor_tile ) const {
  CHECK( quad_exists( q1 ) );
  CHECK( quad_exists( q2 ) );
  maybe<point> const anchor1 =
      find_anchor( q1, is_valid_anchor_tile );
  if( !anchor1.has_value() ) return false;
  maybe<point> const anchor2 =
      find_anchor( q2, is_valid_anchor_tile );
  if( !anchor2.has_value() ) return false;
  return has_path( *anchor1, *anchor2, 6,
                   [this]( point p1, point p2 ) {
                     return are_tiles_connected( p1, p2 );
                   } );
};

template<typename T, typename Func>
void ConnectivityFinder::populate_connectivity(
    array<T, 270>& connectivity,
    Func&&         is_valid_anchor_tile ) const {
  base::ScopedTimer timer( "populate_connectivity: "s +
                           base::demangled_typename<T>() );
  // We support smaller maps than the OG for testing.
  CHECK_GE( int( connectivity.size() ),
            map_height_quads_ * map_width_quads_ );
  auto connectivity_quad = [&]( point q ) -> maybe<T&> {
    if( !quad_exists( q ) ) return nothing;
    int const offset = q.x * map_height_quads_ + q.y;
    CHECK_LT( offset, int( connectivity.size() ) );
    return connectivity[offset];
  };

  for( int qy = 0; qy < map_height_quads_; ++qy ) {
    for( int qx = 0; qx < map_width_quads_; ++qx ) {
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
            are_quads_connected( { .x = qx, .y = qy },
                                 { .x = qx + quad_delta_x,
                                   .y = qy + quad_delta_y },
                                 is_valid_anchor_tile ) )
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
                            size                map_size,
                            CONNECTIVITY&       connectivity ) {
  CHECK_EQ( int( tiles.size() % map_size.w ), 0 );
  CHECK_EQ( int( tiles.size() ), map_size.area() );
  ConnectivityFinder const finder{ .tiles_      = tiles,
                                   .path_       = path,
                                   .map_width_  = map_size.w,
                                   .map_height_ = map_size.h };

  // Sea lane connectivity.
  finder.populate_connectivity(
      connectivity.sea_lane_connectivity, [&]( int offset ) {
        CHECK_LT( offset, int( path.size() ) );
        CHECK_LT( offset, int( tiles.size() ) );
        // _1 is a value of 1 which the OG uses for all tiles
        // that are connected to either the left or right edge of
        // the map, even if the left and right edges are not
        // themselves connected. Note that a region ID of 1 is
        // also used for one of the land masses, but that will be
        // ruled out when we check for water.
        return ( path[offset].region_id ==
                 region_id_4bit_type::_1 ) &&
               is_tile_water( tiles[offset].tile );
      } );

  // Land connectivity.
  finder.populate_connectivity(
      connectivity.land_connectivity, [&]( int offset ) {
        CHECK_LT( offset, int( tiles.size() ) );
        return is_tile_land( tiles[offset].tile );
      } );
}

void populate_region_ids( vector<TILE> const& tiles,
                          vector<PATH>&       path ) {
  path.resize( tiles.size() );
  static PATH const kEmpty{};
  fill( path.begin(), path.end(), kEmpty );
  // TODO
}

} // namespace sav
