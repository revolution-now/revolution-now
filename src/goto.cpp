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
#include "goto-registry.hpp"
#include "harbor-units.hpp"
#include "igoto-viewer.hpp"
#include "igui.hpp"
#include "map-square.hpp"
#include "roles.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/goto.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

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
using ::base::ScopedTimer;
using ::gfx::point;
using ::refl::enum_count;
using ::refl::enum_values;

struct TileWithCost {
  // Must be first for comparison.
  int cost = {};
  // Being diagonal is considered a cost, all else being equal.
  // Note that this only comes into play when the real costs
  // (above) are equal, which is important because a diagonal
  // movement doesn't actually cost more than a cardinal move-
  // ment; we are just treating it like that in order to have the
  // algo prefer cardinal movements, all else being equal, since
  // they look more natural.
  bool is_diagonal = {};
  // To guarantee deterministic sorting when all other sorting
  // parameters are equal.
  point tile = {};

  [[maybe_unused]] auto operator<=>(
      TileWithCost const& ) const = default;
};

template<typename T>
using PriorityQueue =
    priority_queue<T, std::vector<T>, std::greater<T>>;

// NOTE: here we are using a slightly different version of A*
// relative to what is on the wikipedia article. Since our pri-
// ority queue doesn't support updating priorities of nodes in
// the queue (which happens when we find a shorter path to a pre-
// vious tile) we will just re-insert that tile.
GotoPath a_star( IGotoMapViewer const& viewer, point const src,
                 point const dst ) {
  GotoPath goto_path;
  unordered_map<point /*to*/, TileWithCost /*from*/> explored;
  PriorityQueue<TileWithCost> todo;
  auto const push = [&]( point const from, point const to,
                         e_cdirection const d, int const cost ) {
    bool const is_diagonal = to_diagonal( d ).has_value();
    todo.push( { .cost = cost + viewer.heuristic_cost( to, dst ),
                 .is_diagonal = is_diagonal,
                 .tile        = to } );
    explored[to] = {
      .cost = cost, .is_diagonal = is_diagonal, .tile = from };
  };
  push( src, src, e_cdirection::c, 0 );
  while( !todo.empty() ) {
    ++goto_path.meta.iterations;
    point const curr = todo.top().tile;
    todo.pop();
    CHECK( explored.contains( curr ) );
    if( curr == dst ) break;
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = curr.moved( d );
      // This means that, whatever the target tile is, we will
      // allow the unit to at least attempt to enter it. This al-
      // lows e.g. a ship to make landfall or a land unit to at-
      // tack a dwelling which are actions that would normally
      // not be allowed because those units would not normally be
      // able to traverse those tiles (when en-route to their
      // target). In the event that the unit is not allowed onto
      // the tile then that will be detected downstream and the
      // goto orders will be cleared; that is ok because the user
      // specifically chose the target, so it is appropriate e.g.
      // for them to receive a message saying that they cannot
      // enter the tile, which would not be appropriate for tiles
      // en-route. This also allows a ship to move one tile off
      // of the map which means "goto harbor".
      if( moved != dst && !viewer.can_enter_tile( moved ) )
        continue;
      TileWithCost const proposed{
        .cost =
            explored[curr].cost + viewer.travel_cost( curr, d ),
        .is_diagonal = to_diagonal( d ).has_value(),
        .tile        = curr };
      if( auto const it = explored.find( moved );
          it != explored.end() && proposed >= it->second )
        continue;
      // Either we haven't seen this node before or we've found a
      // shorter path to it (see the NOTE above this function re-
      // garding the latter case).
      push( curr, moved, to_cdirection( d ), proposed.cost );
    }
  }
  // Record meta info even if we failed.
  // Note: iterations if filled in above.
  goto_path.meta.queue_size_at_end = todo.size();
  goto_path.meta.tiles_touched     = explored.size();
  if( !explored.contains( dst ) ) return goto_path;
  auto& reverse_path = goto_path.reverse_path;
  reverse_path.reserve( viewer.heuristic_cost( src, dst ) * 9 );
  for( point p = dst; p != src; p = explored[p].tile )
    reverse_path.push_back( p );
  return goto_path;
}

struct TileWithCostSeaLane {
  // Must go in this order for comparison purposes.
  int cost = {};
  // This is to bias our search to the same row that we started
  // in. This way, all else being equal, the search gets biased
  // toward horizontal travel which makes more sense when
  // searching for sea lane. This won't interfere with distance
  // optimization because it will only come into play when two
  // tiles have equal distances.
  int distance_y = {};
  point tile     = {};

  [[maybe_unused]] auto operator<=>(
      TileWithCostSeaLane const& ) const = default;
};

struct ExploredSeaLaneTile {
  point tile = {};
  int cost   = {};
};

GotoPath sea_lane_search( IGotoMapViewer const& viewer,
                          point const src ) {
  GotoPath goto_path;
  unordered_map<point /*to*/, ExploredSeaLaneTile /*from*/>
      explored;
  PriorityQueue<TileWithCostSeaLane> todo;
  auto const push = [&]( point const p, point const from,
                         int const cost ) {
    todo.push( { .cost       = cost,
                 .distance_y = abs( p.y - src.y ),
                 .tile       = p } );
    explored[p] = { .tile = from, .cost = cost };
  };
  push( src, src, 0 );
  while( !todo.empty() ) {
    ++goto_path.meta.iterations;
    point const curr = todo.top().tile;
    CHECK( explored.contains( curr ) );
    if( viewer.is_sea_lane_launch_point( curr ) ) break;
    todo.pop();
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = curr.moved( d );
      if( !viewer.can_enter_tile( moved ) ) continue;
      int const proposed_weight =
          explored[curr].cost + viewer.travel_cost( curr, d );
      if( explored.contains( moved ) ) {
        if( proposed_weight >= explored[moved].cost ) continue;
        // NOTE: this branch never seems to happen in practice
        // for the sea lane search (unlike for land searches),
        // and that is (very likely) because all ocean tiles have
        // the same cost, so this algorithm will be able to find
        // the optimal path without ever processing a given node
        // twice, i.e. it won't every find a better path to a
        // node that was already processed. The A* wiki page men-
        // tions this when discussing heuristic functions being
        // "monotone" or "consistent".
        explored[moved] = { .tile = curr,
                            .cost = proposed_weight };
        // Need to fall through here to reprocess node.
      }
      push( moved, curr, proposed_weight );
    }
  }
  // Record meta info even if we failed.
  // NOTE: iterations is set above.
  goto_path.meta.queue_size_at_end = todo.size();
  goto_path.meta.tiles_touched     = explored.size();
  if( todo.empty() ) return goto_path;
  point const dst = todo.top().tile;
  todo.pop();
  CHECK( explored.contains( dst ) );
  auto& reverse_path = goto_path.reverse_path;
  for( point p = dst; p != src; p = explored[p].tile )
    reverse_path.push_back( p );
  return goto_path;
}

// If no new path is found then the unit will be removed from the
// registry.
void try_new_goto( IGotoMapViewer const& viewer,
                   GotoRegistry& registry, UnitId const unit_id,
                   goto_target const& target,
                   point const unit_tile ) {
  lg.info( "goto: {}", target );
  registry.units.erase( unit_id );
  SWITCH( target ) {
    CASE( map ) {
      point const dst = map.tile;
      auto const goto_path =
          compute_goto_path( viewer, unit_tile, dst );
      if( goto_path.reverse_path.empty() ) break;
      registry.units[unit_id] = GotoExecution{
        .target = target, .path = std::move( goto_path ) };
      break;
    }
    CASE( harbor ) {
      auto const goto_path =
          compute_harbor_goto_path( viewer, unit_tile );
      if( goto_path.reverse_path.empty() ) break;
      registry.units[unit_id] = GotoExecution{
        .target = target, .path = std::move( goto_path ) };
      break;
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
GotoPath compute_goto_path( IGotoMapViewer const& viewer,
                            point const src, point const dst ) {
  GotoPath res;
  ScopedTimer const timer( [&] {
    return format(
        "a-star from {} -> {} finished after exploring {} tiles "
        "with {} iterations",
        src, dst, res.meta.tiles_touched, res.meta.iterations );
  } );
  res = a_star( viewer, src, dst );
  return res;
}

GotoPath compute_harbor_goto_path( IGotoMapViewer const& viewer,
                                   point const src ) {
  GotoPath res;
  ScopedTimer const timer( [&] {
    return format(
        "sea lane search from {} finished after exploring {} "
        "tiles with {} iterations",
        src, res.meta.tiles_touched, res.meta.iterations );
  } );
  res = sea_lane_search( viewer, src );
  return res;
}

bool unit_has_reached_goto_target( SSConst const& ss,
                                   Unit const& unit,
                                   goto_target const& target ) {
  SWITCH( target ) {
    CASE( map ) {
      auto const unit_tile =
          coord_for_unit_indirect( ss.units, unit.id() );
      return unit_tile == map.tile;
    }
    CASE( harbor ) {
      return is_unit_in_port( ss.units, unit.id() );
    }
  }
}

GotoPort find_goto_port( SSConst const& ss,
                         TerrainConnectivity const& connectivity,
                         e_player const player_type,
                         e_unit_type const unit_type,
                         point const src ) {
  GotoPort res;
  bool const is_ship = unit_attr( unit_type ).ship;
  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[player_type] );
  if( is_ship && player.revolution.status <
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
    // where the ship is on the tile to the right of the colony.
    for( e_cdirection const d : enum_values<e_cdirection> ) {
      point const moved = src.moved( d );
      if( !ss.terrain.square_exists( moved ) ) continue;
      if( !is_water( ss.terrain.square_at( moved ) ) ) continue;
      // The following function must only be called with water
      // tiles, since land tiles can have access to the map-edge,
      // but that is not what we want.
      if( water_square_has_ocean_access( connectivity,
                                         moved ) ) {
        res.europe = true;
        break;
      }
    }
  }

  auto const is_compatible_surface = [&]( point const p ) {
    if( is_ship )
      return is_water( ss.terrain.square_at( p ) );
    else
      return is_land( ss.terrain.square_at( p ) );
  };
  static auto const [DONT_ERASE, ERASE] = pair{ false, true };
  res.colonies = ss.colonies.for_player( player_type );
  erase_if( res.colonies, [&]( ColonyId const colony_id ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    if( colony.location.to_gfx() == src )
      // By convention don't display the colony we're sitting on
      // top of.
      return ERASE;
    if( colony.location.direction_to( src ).has_value() )
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
      point const moved = src.moved( d );
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
    GotoPort const& goto_port, e_unit_type const unit_type ) {
  maybe<goto_target> res;
  if( goto_port.colonies.empty() && !goto_port.europe ) {
    co_await gui.message_box(
        "There are no available ports to which we can send this "
        "unit." );
    co_return res;
  }
  ChoiceConfig config;
  config.msg = unit_attr( unit_type ).ship
                   ? "Select Destination Port:"
                   : "Select Destination Colony:";
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
      res = create_goto_map_target(
          ss, player.type, colonies[*choice]->location );
    }
  }
  co_return res;
}

[[nodiscard]] maybe<GotoTargetSnapshot>
compute_goto_target_snapshot( SSConst const& ss,
                              IVisibility const& viz,
                              e_player const unit_player,
                              point const tile ) {
  using S = GotoTargetSnapshot;
  using enum e_tile_visibility;

  e_tile_visibility const visibility = viz.visible( tile );

  // Check if hidden.
  switch( visibility ) {
    case e_tile_visibility::hidden:
      return nothing;
    case e_tile_visibility::fogged:
      break;
    case e_tile_visibility::clear:
      break;
  }

  if( viz.dwelling_at( tile ) ) return S::dwelling{};

  if( auto const colony = viz.colony_at( tile );
      colony.has_value() && colony->player != unit_player )
    return S::foreign_colony{ .player = colony->player };

  if( visibility == clear ) {
    auto const& units = ss.units.from_coord( tile );
    if( !units.empty() ) {
      GenericUnitId const generic_unit_id = *begin( units );
      switch( ss.units.unit_kind( generic_unit_id ) ) {
        case e_unit_kind::native: {
          NativeUnit const& native_unit =
              ss.units.native_unit_for( generic_unit_id );
          e_tribe const tribe_type =
              tribe_type_for_unit( ss, native_unit );
          return S::brave{ .tribe = tribe_type };
        }
        case e_unit_kind::euro: {
          Unit const& unit =
              ss.units.euro_unit_for( generic_unit_id );
          if( unit.player_type() != unit_player )
            return S::foreign_unit{ .player =
                                        unit.player_type() };
          break;
        }
      }
    }
  }

  // This should yield a value because we've already check that
  // the tile is not hidden.
  UNWRAP_CHECK_T( MapSquare const& square,
                  viz.visible_square_at( tile ) );

  // These two should be mutually exclusive in practice.
  if( square.lost_city_rumor ) return S::empty_with_lcr{};
  if( square.sea_lane )
    return S::empty_or_friendly_with_sea_lane{};

  return S::empty_or_friendly{};
}

goto_target::map create_goto_map_target(
    SSConst const& ss, e_player const unit_player,
    point const tile ) {
  // NOTE: we do not look at the omniscient_path_finding game
  // config option here because we want this visibility to really
  // reflect what the player is actually seeing when the choose
  // the target square. The omniscient setting is only then
  // checked when computing the path to that tile.
  auto const viz = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  CHECK( viz );
  return goto_target::map{
    .tile     = tile,
    .snapshot = compute_goto_target_snapshot(
        ss, *viz, unit_player, tile ) };
}

bool is_new_goto_snapshot_allowed(
    maybe<GotoTargetSnapshot> const old,
    GotoTargetSnapshot const& New ) {
  SWITCH( New ) {
    CASE( empty_or_friendly ) { return true; }
    // NOTE: these four below, when compared, will test both the
    // type and that the tribe/player is consistent.
    CASE( foreign_colony ) { return old == foreign_colony; }
    CASE( foreign_unit ) { return old == foreign_unit; }
    CASE( dwelling ) { return old == dwelling; }
    CASE( brave ) { return old == brave; }
    CASE( empty_with_lcr ) {
      // NOTE: this implies the tile is currently empty or
      // friendly.
      return old == empty_with_lcr;
    }
    CASE( empty_or_friendly_with_sea_lane ) {
      // NOTE: this implies the tile is currently empty or
      // friendly. And it is always allowed so long as there were
      // no non-player entities on the tile, though will be
      // checked in the move command handler to decide whether
      // the move should result in a launch onto the high seas.
      return old == nothing ||
             old == empty_or_friendly_with_sea_lane;
    }
  }
}

// NOTE: Here we make it a point not to assume that the unit
// needs to have goto orders per se, because AI units that are
// following paths may not have them. The unit may just have a
// goto_target stored elsewhere in e.g. AI strategy state.
EvolveGoto find_next_move_for_unit_with_goto_target(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    GotoRegistry& registry, IGotoMapViewer const& viewer,
    Unit const& unit, goto_target const& target ) {
  UnitId const unit_id       = unit.id();
  e_player const player_type = unit.player_type();

  auto const abort = [&] {
    registry.units.erase( unit_id );
    return EvolveGoto::abort{};
  };

  if( unit_has_reached_goto_target( ss, unit, target ) )
    return abort();

  // See if we need to get rid of an old goto target if the
  // target has changed. This can happen when a unit is given a
  // new goto order when it didn't complete a previous one. Doing
  // it this way allows us to not need an IAgent method to clear
  // the goto path when a new goto order is given.
  if( registry.units.contains( unit_id ) &&
      registry.units[unit_id].target != target )
    registry.units.erase( unit_id );

  // This should be validated when loading the save, namely
  // that a unit in goto mode must be either directly on the
  // map or in the cargo of another unit that is on the map.
  point const src =
      coord_for_unit_indirect_or_die( ss.units, unit_id );

  // This is the one we will use when we want to see exactly what
  // the player is seeing on screen, and is not necessarily the
  // one being used in the IGotoMapViewer because of the "omni-
  // scient goto" config option.
  auto const real_viz = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  CHECK( real_viz );

  // Here we try twice, and this has two purposes. First, if the
  // unit's goto orders are new and its path hasn't been computed
  // yet, then the first attempt will fail and then we'll compute
  // the path and try again. But it is also needed for a unit
  // that already has a goto path, since as a unit explores
  // hidden tiles it may discover that its current path is no
  // longer viable and may need to recompute a path. But if the
  // second attempt to compute a path still does not succeed then
  // there is not further viable path and we cancel.
  auto const go_or_reattempt =
      [&] [[nodiscard]] (
          auto const& direction_fn ) -> EvolveGoto {
    if( auto const d = direction_fn(); d.has_value() )
      return EvolveGoto::move{ .to = *d };
    try_new_goto( viewer, registry, unit_id, target, src );
    if( auto const d = direction_fn(); d.has_value() )
      return EvolveGoto::move{ .to = *d };
    return abort();
  };

  SWITCH( target ) {
    CASE( map ) {
      auto const direction = [&] -> maybe<e_direction> {
        if( !registry.units.contains( unit_id ) ) return nothing;
        auto& reverse_path =
            registry.units[unit_id].path.reverse_path;
        if( reverse_path.empty() ) return nothing;
        point const dst = reverse_path.back();
        reverse_path.pop_back();
        auto const d = src.direction_to( dst );
        if( !d.has_value() ) return nothing;
        // This means that, whatever the target tile is, we will
        // allow the unit to at least attempt to enter it. This
        // allows e.g. a ship to make landfall or a land unit to
        // attack a dwelling which are actions that would nor-
        // mally not be allowed because those units would not
        // normally be able to traverse those tiles en-route to
        // their target. In the event that the unit is not al-
        // lowed onto the tile then the goto orders will be
        // cleared, but that is ok because the user specifically
        // chose the target.
        if( dst == map.tile ) return *d;
        if( viewer.can_enter_tile( dst ) ) return *d;
        return nothing;
      };

      // It is ok if the destination tile is not on the map; we
      // do allow it to be one tile off the left or right edge to
      // allow the player to send ships to the harbor. The reason
      // we represent such goto commands with a tile and not the
      // dedicated `harbor` mode is because with the latter then
      // you lose the player's desired path to get to the high
      // seas, so e.g. if the unit is on the left side of the map
      // and the player drags a ship off the right edge of the
      // map, we don't want the ship traveling to the left simply
      // because that is a shorter path to the high seas, given
      // that the player explicitly chose the right side.
      bool const dst_exists =
          ss.terrain.square_exists( map.tile );

      if( dst_exists &&
          src.direction_to( map.tile ).has_value() ) {
        // We are adjacent to the destination tile, so let's make
        // sure that the destination tile contains what we
        // thought it did when it was initially chosen by the
        // player. This avoids surprising such as going to an un-
        // explored tile and then finding there is a brave on
        // that tile and automatically attacking it. This should
        // not check-fail because our unit is adjacent to this
        // tile and so it should be clear.
        UNWRAP_CHECK_T(
            GotoTargetSnapshot const new_snapshot,
            compute_goto_target_snapshot(
                ss, *real_viz, player_type, map.tile ) );
        if( !is_new_goto_snapshot_allowed( map.snapshot,
                                           new_snapshot ) ) {
          lg.info(
              "cancelling goto command for {} because the "
              "destination tile contents have changed from [{}] "
              "to [{}] since the command was issued.",
              unit.type(), map.snapshot, new_snapshot );
          return abort();
        }
      }

      return go_or_reattempt( direction );
    }
    CASE( harbor ) {
      auto const direction = [&] -> maybe<e_direction> {
        if( auto const d =
                viewer.is_sea_lane_launch_point( src );
            d.has_value() )
          return *d;
        if( !registry.units.contains( unit_id ) ) return nothing;
        auto& reverse_path =
            registry.units[unit_id].path.reverse_path;
        if( reverse_path.empty() ) return nothing;
        point const dst = reverse_path.back();
        reverse_path.pop_back();
        auto const d = src.direction_to( dst );
        if( !d.has_value() ) return nothing;
        if( viewer.can_enter_tile( dst ) ) return *d;
        return nothing;
      };

      // This is an optimization that tries to take advantage of
      // any new sea lane tiles that the ship reveals as it is
      // making it way along its previously computed path to the
      // sea lane. If there are any sea lane tiles within the
      // ship's visibility at the moment (meaning that they might
      // have been revealed on its last move) we will drop the
      // path which will cause it to be recomputed. We could have
      // computed a new path and then only used it if it were
      // better than the previous one, but that doesn't get us
      // anything because either way we're computing a new op-
      // timal path and ending up with the optimal path.
      if( registry.units.contains( unit_id ) ) {
        vector<Coord> const visible = unit_visible_squares(
            ss, connectivity, player_type, unit.type(), src );
        lg.debug( "re-exploring {} tiles for sea lane.",
                  visible.size() );
        for( point const p : visible ) {
          if( viewer.can_enter_tile( p ) &&
              viewer.is_sea_lane_launch_point( p ) ) {
            lg.debug( "invalidating sea lane search." );
            registry.units.erase( unit_id );
            break;
          }
        }
      }

      return go_or_reattempt( direction );
    }
  }
}

} // namespace rn
