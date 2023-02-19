/****************************************************************
**connectivity.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-17.
*
* Description: Computes connectivity of bodies of land and sea.
*
*****************************************************************/
#include "connectivity.hpp"

// Revolution Now
#include "logger.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// base-util
#include "base-util/stopwatch.hpp"

using namespace std;

namespace rn {

namespace {

void update_terrain_connectivity_impl(
    SSConst const& ss, TerrainConnectivity* out ) {
  int const y_size      = ss.terrain.world_size_tiles().h;
  int const x_size      = ss.terrain.world_size_tiles().w;
  int const total_tiles = y_size * x_size;

  out->x_size = x_size;

  using RastorCoord = int;

  auto rastor = [&]( Coord coord ) {
    return coord.y * x_size + coord.x;
  };

  auto unrastor = [&]( RastorCoord rc ) {
    return Coord{ .x = rc % x_size, .y = rc / x_size };
  };

  auto set_index = [&]( Coord coord, int idx ) {
    int const rastor_coord = rastor( coord );
    CHECK_LT( rastor_coord, int( out->indices.size() ) );
    out->indices[rastor_coord] = idx;
  };

  auto get_index = [&]( Coord coord ) {
    int const rastor_coord = rastor( coord );
    CHECK_LT( rastor_coord, int( out->indices.size() ) );
    return out->indices[rastor_coord];
  };

  // Pre-sizing/reserving. Clear and then reset to fill with ze-
  // roes.
  out->indices.clear();
  out->indices.resize( total_tiles );
  if( total_tiles == 0 ) return;
  out->indices_with_left_edge_access.reserve( total_tiles );
  out->indices_with_right_edge_access.reserve( total_tiles );

  // Use a set to function approximately as a q, since 1) we
  // don't really care the order that things are popped off rela-
  // tive to when they are pushed, and b) we don't want to add
  // duplicates.
  unordered_set<RastorCoord> q;
  q.reserve( total_tiles * 2 );

  auto next_q = [&]() -> Coord {
    CHECK( !q.empty() );
    RastorCoord const next = *q.begin();
    Coord const       res  = unrastor( next );
    q.erase( next );
    return res;
  };

  auto insert_q = [&]( Coord coord ) {
    q.insert( rastor( coord ) );
  };

  auto surface_at = [&]( Coord coord ) {
    return ss.terrain.square_at( coord ).surface;
  };

  // The zero index will never be used; this is done on purpose
  // to catch any cells that were not explored, since they will
  // retain their default-initialized zero value.
  int curr_index = 0;
  for( int y = 0; y < y_size; ++y ) {
    for( int x = 0; x < x_size; ++x ) {
      Coord const seed{ .x = x, .y = y };
      if( get_index( seed ) != 0 )
        // We've already assigned this tile an index.
        continue;
      // This tile has no index, so start an exploration from
      // this tile and label all connected ones with the same in-
      // dex.
      ++curr_index;
      CHECK( q.empty() );
      insert_q( seed );
      do {
        Coord const center = next_q();
        CHECK_EQ( get_index( center ), 0 );
        set_index( center, curr_index );
        e_surface const center_type = surface_at( center );
        for( e_direction const d :
             refl::enum_values<e_direction> ) {
          Coord const moved = center.moved( d );
          if( !ss.terrain.square_exists( moved ) ) continue;
          if( get_index( moved ) > 0 ||
              q.contains( rastor( moved ) ) )
            // This tile has either already been marked with a
            // connectivity index or it is already on the list to
            // search, so no need to search it.
            continue;
          e_surface const moved_type = surface_at( moved );
          if( moved_type == center_type ) insert_q( moved );
        }
      } while( !q.empty() );
    }
  }

  // Now find all of the indices along the left and right edge.
  for( int y = 0; y < y_size; ++y ) {
    Coord const left_edge{ .x = 0, .y = y };
    Coord const right_edge{ .x = x_size - 1, .y = y };
    out->indices_with_left_edge_access.insert(
        get_index( left_edge ) );
    out->indices_with_right_edge_access.insert(
        get_index( right_edge ) );
  }
}

bool contains_segment_index( vector<int> const&        indices,
                             unordered_set<int> const& s,
                             int x_size, Coord coord ) {
  CHECK( !indices.empty(),
         "you must compute the terrain connectivity first." );
  int const rastor_coord = coord.y * x_size + coord.x;
  CHECK_LT( rastor_coord, int( indices.size() ) );
  return s.contains( indices[rastor_coord] );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void update_terrain_connectivity( SSConst const&       ss,
                                  TerrainConnectivity* out ) {
  util::StopWatch     watch;
  static string const kTimerName = "terrain connectivity update";
  watch.start( kTimerName );
  update_terrain_connectivity_impl( ss, out );
  watch.stop( kTimerName );
  lg.debug( "{} took {}.", kTimerName,
            watch.human( kTimerName ) );
}

bool water_square_has_left_ocean_access(
    TerrainConnectivity const& connectivity, Coord coord ) {
  return contains_segment_index(
      connectivity.indices,
      connectivity.indices_with_left_edge_access,
      connectivity.x_size, coord );
}

bool water_square_has_right_ocean_access(
    TerrainConnectivity const& connectivity, Coord coord ) {
  return contains_segment_index(
      connectivity.indices,
      connectivity.indices_with_right_edge_access,
      connectivity.x_size, coord );
}

bool water_square_has_ocean_access(
    TerrainConnectivity const& connectivity, Coord coord ) {
  return water_square_has_left_ocean_access( connectivity,
                                             coord ) ||
         water_square_has_right_ocean_access( connectivity,
                                              coord );
}

bool colony_has_ocean_access(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    Coord tile ) {
  CHECK( ss.terrain.square_at( tile ).surface ==
         e_surface::land );
  for( e_direction const d : refl::enum_values<e_direction> ) {
    Coord const moved = tile.moved( d );
    if( !ss.terrain.square_exists( moved ) ) continue;
    if( ss.terrain.square_at( moved ).surface !=
        e_surface::water )
      continue;
    if( water_square_has_ocean_access( connectivity, moved ) )
      return true;
  }
  return false;
}

} // namespace rn
