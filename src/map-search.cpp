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
#include "ss/natives.hpp"
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
  int len    = 1;
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
  auto const points = rl::all( spiral_gen )
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
    double distance    = {};
  };
  maybe<ColonyWithDistance> state;
  // Iterate through colonies by ID so that we get a determin-
  // istic ordering; otherwise we'd be iterating over the un-
  // ordered map in which the colonies are store.
  for( ColonyId colony_id = ColoniesState::kFirstColonyId;
       colony_id <= *last; ++colony_id ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    double const distance =
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

maybe<Colony const&> find_close_explored_colony(
    SSConst const& ss, e_player player, point location,
    double max_distance ) {
  VisibilityForPlayer const viz( ss, player );
  base::generator<point> const search =
      outward_spiral_pythdist_search_existing_gen(
          ss, location, max_distance );
  for( point const point : search ) {
    maybe<Colony const&> colony =
        viz.colony_at( Coord::from_gfx( point ) );
    if( colony.has_value() ) return colony;
  }
  return nothing;
}

// Yields a finite stream of friendly colonies spiraling outward
// from the starting point that are within a radius of
// `max_distance` to the start.
vector<ColonyId> close_friendly_colonies( SSConst const& ss,
                                          e_player player,
                                          gfx::point const start,
                                          double max_distance ) {
  vector<ColonyId> res;
  generator<gfx::point> points =
      outward_spiral_pythdist_search_existing_gen(
          ss, start, max_distance );
  for( gfx::point p : points ) {
    Coord const square = Coord::from_gfx( p );
    // Is there a friendly colony there.
    maybe<ColonyId> const colony_id =
        ss.colonies.maybe_from_coord( square );
    if( !colony_id.has_value() ) continue;
    if( ss.colonies.colony_for( *colony_id ).player != player )
      continue;
    res.push_back( *colony_id );
  }
  return res;
}

maybe<e_tribe> find_close_encountered_tribe(
    SSConst const& ss, e_player player, gfx::point location,
    double max_distance ) {
  generator<gfx::point> const points =
      outward_spiral_pythdist_search_existing_gen(
          ss, location, max_distance );
  for( gfx::point const p : points ) {
    Coord const square = Coord::from_gfx( p );
    // Is there a tribe there that we've met?
    maybe<DwellingId> const dwelling_id =
        ss.natives.maybe_dwelling_from_coord( square );
    if( !dwelling_id.has_value() ) continue;
    Tribe const& tribe = ss.natives.tribe_for( *dwelling_id );
    if( !tribe.relationship[player].encountered ) continue;
    return tribe.type;
  }
  return nothing;
}

} // namespace rn
