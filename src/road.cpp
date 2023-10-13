/****************************************************************
**road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: Road rendering and state changes.
*
*****************************************************************/
#include "road.hpp"

// Revolution Now
#include "imap-updater.hpp"
#include "logger.hpp"
#include "tiles.hpp"
#include "visibility.hpp"

// config
#include "config/command.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// render
#include "render/painter.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <array>

using namespace std;

namespace rn {

namespace {

int turns_required( e_unit_type unit_type, e_terrain terrain ) {
  int for_terrain = config_command.road_turns[terrain];
  switch( unit_type ) {
    case e_unit_type::pioneer:
      return for_terrain;
    case e_unit_type::hardy_pioneer:
      return std::max( for_terrain / 2, 1 );
    default:
      break;
  }
  FATAL( "unit type {} cannot build a road.", unit_type );
}

} // namespace

/****************************************************************
** Road State
*****************************************************************/
void set_road( IMapUpdater& map_updater, Coord tile ) {
  map_updater.modify_map_square(
      tile, []( MapSquare& square ) { square.road = true; } );
}

void clear_road( IMapUpdater& map_updater, Coord tile ) {
  map_updater.modify_map_square(
      tile, []( MapSquare& square ) { square.road = false; } );
}

bool has_road( MapSquare const& square ) { return square.road; }

bool has_road( TerrainState const& terrain_state, Coord tile ) {
  return has_road( terrain_state.square_at( tile ) );
}

/****************************************************************
** Unit State
*****************************************************************/
void perform_road_work( UnitsState const&   units_state,
                        TerrainState const& terrain_state,
                        Player const&       player,
                        IMapUpdater& map_updater, Unit& unit ) {
  Coord location = units_state.coord_for( unit.id() );
  UNWRAP_CHECK( road_orders,
                unit.orders().get_if<unit_orders::road>() );
  CHECK( unit.type() == e_unit_type::pioneer ||
             unit.type() == e_unit_type::hardy_pioneer,
         "unit type {} should not be building a road.",
         unit.type() );
  CHECK_GT( unit.movement_points(), 0 );
  auto log = [&]( string_view status ) {
    lg.debug( "road work {} for unit {} with {} tools left.",
              status, debug_string( unit ),
              unit.composition()[e_unit_inventory::tools] );
  };

  // First check if there is already a road on this square.
  // This could happen if there are two units building a road
  // on the same square and the other unit finished first. In
  // that case, we will just clear this unit's orders and not
  // charge it any tools.
  if( has_road( terrain_state, location ) ) {
    log( "cancelled" );
    unit.clear_orders();
    return;
  }
  // The unit is still building the road.
  int const turns_worked = road_orders.turns_worked;
  int const road_turns   = turns_required(
      unit.type(),
      effective_terrain( terrain_state.square_at( location ) ) );
  CHECK_LE( turns_worked, road_turns );
  if( turns_worked == road_turns ) {
    // We're finished building the road.
    set_road( map_updater, location );
    unit.clear_orders();
    unit.consume_20_tools( player );
    log( "finished" );
    return;
  }
  // We need more work.
  log( "ongoing" );
  unit.forfeight_mv_points();
  ++road_orders.turns_worked;
}

bool can_build_road( Unit const& unit ) {
  return unit.type() == e_unit_type::pioneer ||
         unit.type() == e_unit_type::hardy_pioneer;
}

/****************************************************************
** Rendering
*****************************************************************/
void render_road_if_present( rr::Painter& painter, Coord where,
                             SSConst const&     ss,
                             IVisibility const& viz,
                             Coord              world_tile ) {
  auto has_road = [&]( Coord tile ) {
    return viz.visible( tile ) != e_tile_visibility::hidden &&
           rn::has_road( viz.square_at( tile ) );
  };

  if( !has_road( world_tile ) ) return;

  static constexpr array<pair<e_direction, e_tile>, 8> const
      road_tiles{
          pair{ e_direction::nw, e_tile::road_nw },
          pair{ e_direction::n, e_tile::road_n },
          pair{ e_direction::ne, e_tile::road_ne },
          pair{ e_direction::e, e_tile::road_e },
          pair{ e_direction::se, e_tile::road_se },
          pair{ e_direction::s, e_tile::road_s },
          pair{ e_direction::sw, e_tile::road_sw },
          pair{ e_direction::w, e_tile::road_w },
      };

  bool road_in_surroundings = false;
  for( auto [direction, tile] : road_tiles ) {
    Coord shifted = world_tile.moved( direction );
    // If the square is off the map then this should yield one of
    // the proto squares which will not have a road, which is
    // what we want.
    if( !has_road( shifted ) ) continue;
    road_in_surroundings = true;
    render_sprite( painter, where, tile );
  }
  if( !road_in_surroundings ) {
    // Native dwellings have roads under them, but they don't re-
    // ally look good with a large "road island" under them,
    // which happens when there are no adjacent roads. So we will
    // render the smaller road island so that it will be com-
    // pletely hidden behind the dwelling, but still be visible
    // e.g. in "hidden terrain" view or while in the map editor.
    bool const has_dwelling =
        ss.natives.maybe_dwelling_from_coord( world_tile )
            .has_value();
    if( !has_dwelling )
      render_sprite( painter, where, e_tile::road_island );
    else
      render_sprite( painter, where, e_tile::road_island_small );
  }
}

} // namespace rn
