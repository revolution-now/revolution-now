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
#include "co-wait.hpp"
#include "connectivity.hpp"
#include "harbor-units.hpp"
#include "igoto-viewer.hpp"
#include "igui.hpp"
#include "map-square.hpp"
#include "unit-mgr.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"

// C++ standard library
#include <queue>
#include <ranges>
#include <unordered_map>

using namespace std;

namespace rg = std::ranges;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::refl::enum_count;
using ::refl::enum_values;

struct TileWithCost {
  point tile = {};
  int cost   = {};

  [[maybe_unused]] friend bool operator<(
      TileWithCost const l, TileWithCost const r ) {
    if( l.cost != r.cost ) return l.cost > r.cost;
    // These only affect the path in trivial ways (i.e. switching
    // among equally optimal paths) but we include it because it
    // makes things more deterministic for unit tests across dif-
    // ferent std libs.
    if( l.tile.y != r.tile.y ) return l.tile.y > r.tile.y;
    if( l.tile.x != r.tile.x ) return l.tile.x > r.tile.x;
    return false;
  };
};

struct ExploredTile {
  point tile = {};
  int cost   = {};
};

GotoPath a_star( IGotoMapViewer const& viewer, point const src,
                 point const dst ) {
  GotoPath goto_path;
  unordered_map<point /*to*/, ExploredTile /*from*/> explored;
  priority_queue<TileWithCost> todo;

  base::ScopedTimer const timer(
      format( "a-star from {} -> {}", src, dst ) );

  auto const push = [&]( point const from, point const to,
                         int const cost ) {
    todo.push(
        { .tile = to,
          .cost = cost + viewer.heuristic_cost( to, dst ) } );
    explored[to] = { .tile = from, .cost = cost };
  };
  push( src, src, 0 );
  int iterations = 0;
  while( !todo.empty() ) {
    ++iterations;
    point const curr = todo.top().tile;
    CHECK( explored.contains( curr ) );
    todo.pop();
    if( curr == dst ) break;
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = curr.moved( d );
      // This means that, whatever the target tile is, we will
      // allow the unit to at least attempt to enter it. This al-
      // lows e.g. a ship to make landfall or a land unit to at-
      // tack a dwelling which are actions that would normally
      // not be allowed because those units would not normally be
      // able to traverse those tiles en-route to their target.
      // In the event that the unit is not allowed onto the tile
      // then the goto orders will be cleared, but that is ok be-
      // cause the user specifically chose the target. This also
      // allows a ship to move one tile off of the map which is
      // used to signal "goto harbor".
      if( moved != dst && !viewer.can_enter_tile( moved ) )
        continue;
      int const proposed_cost =
          explored[curr].cost + viewer.travel_cost( curr, d );
      if( auto const it = explored.find( moved );
          it != explored.end() &&
          proposed_cost >= it->second.cost )
        continue;
      push( curr, moved, proposed_cost );
    }
  }
  lg.debug(
      "a-star from {} -> {} finished after exploring {} tiles "
      "with {} iterations.",
      src, dst, explored.size(), iterations );
  auto& meta             = goto_path.meta;
  meta.iterations        = iterations;
  meta.queue_size_at_end = todo.size();
  meta.tiles_touched     = explored.size();
  if( !explored.contains( dst ) ) return goto_path;
  auto& reverse_path = goto_path.reverse_path;
  reverse_path.reserve( ( src - dst ).chessboard_distance() *
                        9 );
  for( point p = dst; p != src; p = explored[p].tile )
    reverse_path.push_back( p );
  return goto_path;
}

struct TileWithCostSeaLane {
  point tile = {};
  int cost   = {};
  // Used only by the sea lane search to bias our search to the
  // same row that we started in. This way, all else being equal,
  // the search gets biased toward horizontal travel which makes
  // more sense when searching for sea lane. This won't interfere
  // with distance optimization because it will only come into
  // play when two tiles have equal distances.
  int distance_y = {};

  [[maybe_unused]] friend bool operator<(
      TileWithCostSeaLane const l,
      TileWithCostSeaLane const r ) {
    if( l.cost != r.cost ) return l.cost > r.cost;
    if( l.distance_y != r.distance_y )
      return l.distance_y > r.distance_y;
    // These only affect the path in trivial ways (i.e. switching
    // among equally optimal paths) but we include it because it
    // makes things more deterministic for unit tests across dif-
    // ferent std libs.
    if( l.tile.y != r.tile.y ) return l.tile.y > r.tile.y;
    if( l.tile.x != r.tile.x ) return l.tile.x > r.tile.x;
    return false;
  };
};

GotoPath sea_lane_search( IGotoMapViewer const& viewer,
                          point const src ) {
  GotoPath goto_path;
  unordered_map<point /*to*/, ExploredTile /*from*/> explored;
  priority_queue<TileWithCostSeaLane> todo;
  auto const push = [&]( point const p, point const from,
                         int const cost ) {
    todo.push( { .tile       = p,
                 .cost       = cost,
                 .distance_y = abs( p.y - src.y ) } );
    explored[p] = ExploredTile{ .tile = from, .cost = cost };
  };
  push( src, src, 0 );
  base::ScopedTimer const timer(
      format( "sea lane search from {}", src ) );
  maybe<point> dst;
  int iterations = 0;
  while( !todo.empty() ) {
    ++iterations;
    point const curr = todo.top().tile;
    todo.pop();
    if( viewer.is_sea_lane_launch_point( curr ) ) {
      dst = curr;
      break;
    }
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = curr.moved( d );
      if( !viewer.can_enter_tile( moved ) ) continue;
      CHECK( explored.contains( curr ) );
      int const proposed_weight =
          explored[curr].cost + viewer.travel_cost( curr, d );
      if( explored.contains( moved ) ) {
        if( proposed_weight < explored[moved].cost )
          explored[moved] = ExploredTile{
            .tile = curr, .cost = proposed_weight };
        continue;
      }
      push( moved, curr, proposed_weight );
    }
  }
  lg.debug(
      "sea lane search from {} finished after exploring {} "
      "tiles.",
      src, explored.size() );
  auto& meta             = goto_path.meta;
  meta.iterations        = iterations;
  meta.queue_size_at_end = todo.size();
  meta.tiles_touched     = explored.size();
  if( !dst.has_value() ) return goto_path;
  CHECK( explored.contains( *dst ) );
  auto& reverse_path = goto_path.reverse_path;
  for( point p = *dst; p != src; p = explored[p].tile )
    reverse_path.push_back( p );
  return goto_path;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
GotoPath compute_goto_path( IGotoMapViewer const& viewer,
                            point const src, point const dst ) {
  return a_star( viewer, src, dst );
}

GotoPath compute_harbor_goto_path( IGotoMapViewer const& viewer,
                                   point const src ) {
  return sea_lane_search( viewer, src );
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
      return unit_tile == map.tile;
    }
    CASE( harbor ) {
      return is_unit_inbound( ss.units, unit.id() ) ||
             is_unit_in_port( ss.units, unit.id() );
    }
  }
}

GotoPort find_goto_port( SSConst const& ss,
                         TerrainConnectivity const& connectivity,
                         Unit const& unit ) {
  GotoPort res;
  point const tile =
      coord_for_unit_indirect_or_die( ss.units, unit.id() );
  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[unit.player_type()] );
  if( unit.desc().ship && player.revolution.status <
                              e_revolution_status::declared ) {
    // We need to test all tiles around the ship to see if any
    // have sea lane access (as opposed to just testing the tile
    // that the unit is on) because we may have a ship sitting in
    // a port colony, in which case that very tile would not have
    // ocean access, but the ship would. And then we need to in-
    // clude the center tile in case we have a map like this:
    //
    //                      _ _ _ _ _ _
    //                      _ _ _ _ _ _
    //                      _ _ _ _ X X
    //                      _ _ _ _ C _
    //                      _ _ _ _ X X
    //                      _ _ _ _ _ _
    //
    // Where the colony might want to send a ship to the right.
    for( e_cdirection const d : enum_values<e_cdirection> ) {
      point const moved = tile.moved( d );
      if( water_square_has_ocean_access( connectivity,
                                         moved ) ) {
        res.europe = true;
        break;
      }
    }
  }

  res.colonies = ss.colonies.for_player( unit.player_type() );
  auto const is_compatible_surface = [&]( point const p ) {
    if( unit.desc().ship )
      return is_water( ss.terrain.square_at( p ) );
    else
      return is_land( ss.terrain.square_at( p ) );
  };
  static auto const [DONT_ERASE, ERASE] = pair{ false, true };
  erase_if( res.colonies, [&]( ColonyId const colony_id ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    if( colony.location.to_gfx() == tile )
      // By convention don't display the colony we're sitting on
      // top of.
      return ERASE;
    if( colony.location.direction_to( tile ).has_value() )
      // If we either in the colony or adjacent to it then we can
      // always move into it, regardless of unit types or terrain
      // types.
      return DONT_ERASE;
    // The tiles are not adjacent, so the unit would have to
    // move at least one tile to get to the colony. So for each
    // tile around the unit (that it can move to just based on
    // the land/water status, ignoring anything that is on
    // that tile) see if that tile is connected to any of the
    // tiles in or around the colony.  If any such two are
    // connected then the unit can make it to the colony. This
    // is a bit convoluted but is needed to handle all of the
    // various cases:
    //
    //   * Ship in colony sailing to a colony on another conti-
    //     nent.
    //   * Ship in ocean sailing to a colony (which is on land).
    //   * Ship in a colony on land should not be able to get to
    //     another colony on that same land mass.
    //   * Land unit in a colony should not be able to get to a
    //     colony on another continent, even if accessible by wa-
    //     ter.
    //
    // NOTE: in what follows, we can look at the terrain directly
    // here because we are only looking at tiles surrounding the
    // colony and unit, both of which belong to the player, so
    // the player will always have clear visibility on.
    using D = e_cdirection;
    vector<point> src_tiles, dst_tiles;
    src_tiles.reserve( enum_count<D> );
    dst_tiles.reserve( enum_count<D> );
    for( D const d : enum_values<D> ) {
      point const moved = tile.moved( d );
      if( !ss.terrain.square_exists( moved ) ) continue;
      if( !is_compatible_surface( moved ) )
        // If a land unit is on a ship or a ship is in a colony
        // then this will exclude that tile, because we don't
        // want those units using that surface type to get to the
        // destination for the purposes of the intermediate tiles
        // on the way from source to destination.
        continue;
      src_tiles.push_back( moved );
    }
    for( D const d : enum_values<D> ) {
      point const moved = colony.location.moved( d );
      if( !ss.terrain.square_exists( moved ) ) continue;
      dst_tiles.push_back( moved );
    }
    return has_overlapping_connectivity( connectivity, src_tiles,
                                         dst_tiles )
               ? DONT_ERASE
               : ERASE;
  } );
  // This not only creates determinism for unit testing, but also
  // puts the colonies in order of earliest founded to latest,
  // which is what the OG does an probably makes sense to the
  // player.
  rg::sort( res.colonies );
  return res;
}

wait<maybe<goto_target>> ask_goto_port(
    SSConst const& ss, IGui& gui, Player const& player,
    GotoPort const& goto_port ) {
  maybe<goto_target> res;
  if( goto_port.colonies.empty() && !goto_port.europe ) {
    co_await gui.message_box(
        "There are no available ports to which we can send this "
        "unit." );
    co_return res;
  }
  ChoiceConfig config;
  config.msg = "Select Destination Port:";
  static string constexpr kEuropeKey = " <europe> ";
  if( goto_port.europe ) {
    string const harbor_str = format(
        "{} ({})",
        config_nation.nations[player.nation].harbor_city_name,
        config_nation.nations[player.nation].country_name );
    // A key of zero will never be used by the colonies below.
    config.options.push_back( ChoiceConfigOption{
      .key = kEuropeKey, .display_name = harbor_str } );
  }
  unordered_map<string, Colony const*> colonies;
  for( ColonyId const colony_id : goto_port.colonies ) {
    CHECK( colony_id != 0 );
    Colony const& colony = ss.colonies.colony_for( colony_id );
    string const key     = to_string( colony_id );
    colonies[key]        = &colony;
    config.options.push_back( ChoiceConfigOption{
      .key = key, .display_name = colony.name } );
  }
  auto const choice = co_await gui.optional_choice( config );
  if( choice.has_value() ) {
    if( *choice == kEuropeKey ) {
      res = goto_target::harbor{};
    } else {
      CHECK( colonies.contains( *choice ) );
      CHECK( colonies[*choice] );
      res = goto_target::map{ .tile =
                                  colonies[*choice]->location };
    }
  }
  co_return res;
}

} // namespace rn
