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

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::generator;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
generator<gfx::point> outward_spiral_search(
    gfx::point const start ) {
  gfx::point curr = start;
  int        len  = 1;
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

generator<gfx::point> outward_spiral_search_existing(
    SSConst const& ss, gfx::point const start ) {
  int remaining = ss.terrain.world_size_tiles().area();
  for( gfx::point const p : outward_spiral_search( start ) ) {
    if( !ss.terrain.square_exists( Coord::from_gfx( p ) ) )
      continue;
    co_yield p;
    --remaining;
    if( remaining == 0 ) break;
  }
}

} // namespace rn
