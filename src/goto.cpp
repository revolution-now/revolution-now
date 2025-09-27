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

// Revolution Now
#include "igoto-viewer.hpp"
#include "unit-mgr.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit.hpp"

// rds
#include "rds/switch-macro.hpp"

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

struct TileWithSteps {
  point tile = {};
  int steps  = {};
};

maybe<GotoPath> a_star( IGotoMapViewer const& viewer,
                        point const src, point const dst ) {
  maybe<GotoPath> res;
  unordered_map<point /*to*/, TileWithSteps /*from*/> explored;
  priority_queue<TileWithDistance> todo;
  auto const push = [&]( point const p, point const from,
                         int const steps ) {
    todo.push( TileWithDistance{
      .tile     = p,
      .distance = steps + ( p - dst ).pythagorean() } );
    explored[p] = TileWithSteps{ .tile = from, .steps = steps };
  };
  push( src, src, 0 );
  while( !todo.empty() ) {
    point const curr = todo.top().tile;
    todo.pop();
    if( curr == dst ) break;
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = curr.moved( d );
      if( !viewer.can_enter_tile( moved ) ) continue;
      CHECK( explored.contains( curr ) );
      int const proposed_steps = explored[curr].steps + 1;
      if( explored.contains( moved ) ) {
        if( proposed_steps < explored[moved].steps )
          explored[moved] = TileWithSteps{
            .tile = curr, .steps = proposed_steps };
        continue;
      }
      push( moved, curr, proposed_steps );
    }
  }
  lg.debug( "a-star: {} -> {} finished after touching {} tiles.",
            src, dst, explored.size() );
  if( !explored.contains( dst ) ) return res;
  auto& reverse_path = res.emplace().reverse_path;
  for( point p = dst; p != src; p = explored[p].tile )
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

bool unit_has_reached_goto_target( SSConst const& ss,
                                   Unit const& unit ) {
  auto const go_to = unit.orders().get_if<unit_orders::go_to>();
  if( !go_to.has_value() ) return false;
  SWITCH( go_to->target ) {
    CASE( map ) {
      // It should be validated when loading a save that any unit
      // with goto->map orders should be on the map at least in-
      // directly, and the game should maintain that.
      point const unit_tile =
          coord_for_unit_indirect_or_die( ss.units, unit.id() );
      return ( unit_tile == map.tile );
    }
  }
}

} // namespace rn
