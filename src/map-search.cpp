/****************************************************************
**map-search.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Algorithms for searching the map.
*
*****************************************************************/
#include "map-search.hpp"

// Revolution Now
#include "maybe.hpp"
#include "visibility.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/fog-square.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// base
#include "base/generator.hpp"
#include "base/range-lite.hpp"

using namespace std;

namespace rl = base::rl;

namespace rn {

namespace {

using ::base::generator;
using ::gfx::point;

// NOTE: we want all of the coroutines in this module to be
// static (internal only) because that allows the optimizer to
// better optimize them, since it knows all possible ways they
// will be used.
#define YIELD( r ) static generator<r>

// Yields an infinite stream of points spiraling outward from
// the starting point, i.e. (a-y):
//
//   j k l m n
//   y b c d o
//   x i a e p
//   w h g f q
//   v u t s r
//
YIELD( point ) outward_spiral_search( point const start ) {
  point curr = start;
  int   len  = 1;
  co_yield curr;
  while( true ) {
    --curr.x;
    --curr.y;
    len += 2;
    // Top edge, right edge, bottom edge, left edge.
    for( int i = 0; i < len - 1; ++i, ++curr.x ) co_yield curr;
    for( int i = 0; i < len - 1; ++i, ++curr.y ) co_yield curr;
    for( int i = 0; i < len - 1; ++i, --curr.x ) co_yield curr;
    for( int i = 0; i < len - 1; ++i, --curr.y ) co_yield curr;
  }
}

// Same as above by limits the search to squares within the given
// pythagorean distance. A distance of zero will include the
// starting square. A distance of one will include the four car-
// dinally adjacent squares, etc.
YIELD( point )
outward_spiral_pythdist_search_existing_gen(
    SSConst const& ss, point const start, double max_distance ) {
  auto const spiral_gen = outward_spiral_search( start );
  // If we search a NxN grid then we should cover all of the ones
  // that are within the requested pythagorean distance to the
  // starting square.
  int const N = static_cast<int>( max_distance * 2 + 1 );
  int const kMaxSquaresToSearch = N * N;

  auto close_enough = [&]( point p ) {
    return ( start - p ).pythagorean() <= max_distance;
  };

  auto exists = [&]( point p ) {
    return ss.terrain.square_exists( Coord::from_gfx( p ) );
  };

  // To avoid an infinite search, we need to call `take` before
  // we call `exists`.
  auto points = rl::all( spiral_gen )
                    .take( kMaxSquaresToSearch )
                    .keep_if( exists )
                    .keep_if( close_enough );

  for( point const point : points ) co_yield point;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
vector<point> outward_spiral_pythdist_search_existing(
    SSConst const& ss, point const start, double max_distance ) {
  vector<point> res;
  for( point const p :
       outward_spiral_pythdist_search_existing_gen(
           ss, start, max_distance ) )
    res.push_back( p );
  return res;
}

// The idea with the implementation here is that we iterate over
// colonies instead of squares with the assumption that there
// will generally by fewer colonies than squares, even if we are
// e.g. iterating over a 7x7 grid.
maybe<Colony const&> find_any_close_colony(
    SSConst const& ss, gfx::point location, double max_distance,
    base::function_ref<bool( Colony const& )> pred ) {
  maybe<ColonyId> const last = ss.colonies.last_colony_id();
  if( !last.has_value() ) return nothing;
  struct ColonyWithDistance {
    ColonyId colony_id = {};
    double   distance  = {};
  };
  maybe<ColonyWithDistance> state;
  // Iterate through colonies by ID so that we get a determin-
  // istic ordering; otherwise we'd be iterating over the un-
  // ordered map in which the colonies are store.
  for( ColonyId colony_id = ColoniesState::kFirstColonyId;
       colony_id <= *last; ++colony_id ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    double const  distance =
        ( colony.location - Coord::from_gfx( location ) )
            .diagonal();
    if( distance > max_distance ) continue;
    if( !pred( colony ) ) continue;
    if( !state.has_value() || distance < state->distance )
      state = ColonyWithDistance{ .colony_id = colony_id,
                                  .distance  = distance };
  }
  return state.fmap(
      [&]( ColonyWithDistance const& cwd ) -> Colony const& {
        return ss.colonies.colony_for( cwd.colony_id );
      } );
}

// Note that when a tile is visible we use the real square in-
// stead of the fog square because the contents of fog squares
// are not guaranteed to be current with the real square when the
// square is visible and clear. Fog squares only get updated when
// a square goes from visible to fogged.
maybe<ExploredColony> find_close_explored_colony(
    SSConst const& ss, e_nation nation, point location,
    double max_distance ) {
  UNWRAP_CHECK( player_terrain,
                ss.terrain.player_terrain( nation ) );
  base::generator<point> const search =
      outward_spiral_pythdist_search_existing_gen(
          ss, location, max_distance );
  VisibilityForNation const viz( ss, nation );
  for( point const point : search ) {
    switch( viz.visible( Coord::from_gfx( point ) ) ) {
      case e_tile_visibility::hidden:
        continue;
      case e_tile_visibility::visible_and_clear: {
        maybe<ColonyId> const colony_id =
            ss.colonies.maybe_from_coord(
                Coord::from_gfx( point ) );
        if( !colony_id.has_value() ) continue;
        Colony const& colony =
            ss.colonies.colony_for( *colony_id );
        return ExploredColony{ .name     = colony.name,
                               .location = colony.location };
      }
      case e_tile_visibility::visible_with_fog: {
        maybe<FogSquare> const& fog_square =
            player_terrain.map[Coord::from_gfx( point )];
        if( !fog_square.has_value() )
          // The generator should be giving us only squares that
          // exist on the map, so this here means that the square
          // is unexplored.
          continue;
        if( !fog_square->colony.has_value() )
          // No colony here the last time we explored.
          continue;
        FogColony const& fog_colony = *fog_square->colony;
        return ExploredColony{ .name     = fog_colony.name,
                               .location = fog_colony.location };
      }
    }
  }
  return nothing;
}

// Yields a finite stream of friendly colonies spiraling outward
// from the starting point that are within a radius of
// `max_distance` to the start.
vector<ColonyId> close_friendly_colonies( SSConst const& ss,
                                          Player const&  player,
                                          gfx::point const start,
                                          double max_distance ) {
  vector<ColonyId>      res;
  generator<gfx::point> points =
      outward_spiral_pythdist_search_existing_gen(
          ss, start, max_distance );
  for( gfx::point p : points ) {
    Coord const square = Coord::from_gfx( p );
    // Is there a friendly colony there.
    maybe<ColonyId> const colony_id =
        ss.colonies.maybe_from_coord( square );
    if( !colony_id.has_value() ) continue;
    if( ss.colonies.colony_for( *colony_id ).nation !=
        player.nation )
      continue;
    res.push_back( *colony_id );
  }
  return res;
}

} // namespace rn
