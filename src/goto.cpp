/****************************************************************
**goto.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-25.
*
* Description: Goto-related things.
*
*****************************************************************/
#include "goto.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

// C++ standard library
#include <queue>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::refl::enum_values;

struct TileWithDistance {
  point tile      = {};
  double distance = {};

  [[maybe_unused]] friend bool operator<(
      TileWithDistance const l, TileWithDistance const r ) {
    if( l.distance != r.distance )
      return l.distance > r.distance;
    if( l.tile.y != r.tile.y ) return l.tile.y < r.tile.y;
    if( l.tile.x != r.tile.x ) return l.tile.x < r.tile.x;
    return false;
  };
};

maybe<GotoPath> a_star( IGotoMapViewer const& viewer,
                        point const src, point const dst ) {
  maybe<GotoPath> res;
  unordered_map<point /*to*/, point /*from*/> explored;
  priority_queue<TileWithDistance> todo;
  auto const push = [&]( point const p, point const from ) {
    todo.push( TileWithDistance{
      .tile = p, .distance = ( p - dst ).pythagorean() } );
    explored[p] = from;
  };
  push( src, src );
  while( !todo.empty() ) {
    point const curr = todo.top().tile;
    todo.pop();
    if( curr == dst ) break;
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = curr.moved( d );
      if( explored.contains( moved ) ) continue;
      if( !viewer.can_enter_tile( moved ) ) continue;
      push( moved, curr );
    }
  }
  lg.debug( "a-star: {} -> {} finished after touching {} tiles.",
            src, dst, explored.size() );
  if( !explored.contains( dst ) ) return res;
  auto& reverse_path = res.emplace().reverse_path;
  for( point p = dst; p != src; p = explored[p] )
    reverse_path.push_back( p );
  return res;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
maybe<GotoPath> compute_goto_path( IGotoMapViewer const& viewer,
                                   point const src,
                                   point const dst ) {
  if( !viewer.can_enter_tile( dst ) ) return nothing;
  return a_star( viewer, src, dst );
}

} // namespace rn
